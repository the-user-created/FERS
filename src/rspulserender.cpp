//rspulserender.cpp
//Performs the second phase of the simulation - rendering the result
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//7 June 2006

#define TIXML_USE_STL

#include "rspulserender.h"

#include <cstdio> //The C stdio functions are used for binary export
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tinyxml.h>
#include <boost/scoped_array.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include "rsdsp.h"
#include "rshdf5.h"
#include "rsnoise.h"
#include "rsparameters.h"
#include "rsportable.h"
#include "rsradar.h"
#include "rsresponse.h"
#include "rssignal.h"
#include "rstiming.h"

using namespace rs;

namespace
{
	///Open the binary file for response export
	long int openHdf5File(const std::string& recvName)
	{
		long int hdf5_file = 0;
		if (RsParameters::exportBinary())
		{
			//Build the filename for the binary file
			std::ostringstream b_oss;
			b_oss.setf(std::ios::scientific);
			b_oss << recvName << ".h5";
			//Open the binary file for writing
			hdf5_file = rshdf5::createFile(b_oss.str().c_str());
		}
		return hdf5_file;
	}

	/// Add noise to the window, to simulate receiver noise
	void addNoiseToWindow(RsComplex* data, const unsigned int size, const RS_FLOAT temperature)
	{
		if (temperature == 0)
		{
			return;
		}
		//Calculate the noise power
		const RS_FLOAT power = rs_noise::noiseTemperatureToPower(temperature,
		                                                        RsParameters::rate() * RsParameters::oversampleRatio() /
		                                                        2);
		WgnGenerator generator(sqrt(power) / 2.0);
		//Add the noise to the signal
		for (unsigned int i = 0; i < size; i++)
		{
			data[i] += RsComplex(generator.getSample(), generator.getSample());
		}
	}

	/// Normalize a window, and quantize the signal as required
	RS_FLOAT quantizeWindow(RsComplex* data, const unsigned int size)
	{
		RS_FLOAT max = 0;
		//Get the full scale amplitude of the pulse
		for (unsigned int i = 0; i < size; i++)
		{
			if (std::fabs(data[i].real()) > max)
			{
				max = std::fabs(data[i].real());
			}
			if (std::fabs(data[i].imag()) > max)
			{
				max = std::fabs(data[i].imag());
			}
			if (std::isnan(data[i].real()) || std::isnan(data[i].imag()))
			{
				throw std::runtime_error("NAN in QuantizeWindow -- early");
			}
		}
		if (RsParameters::adcBits() > 0)
		{
			//Simulate quantization and normalize
			rs_signal::adcSimulate(data, size, RsParameters::adcBits(), max);
		}
		else
		{
			//Just normalize
			if (max != 0)
			{
				for (unsigned int i = 0; i < size; i++)
				{
					data[i] /= max;
					if (std::isnan(data[i].real()) || std::isnan(data[i].imag()))
					{
						throw std::runtime_error("NAN in QuantizeWindow -- late");
					}
				}
			}
		}
		//Return the full scale value
		return max;
	}

	/// Add a response array to the receive window
	void addArrayToWindow(const RS_FLOAT wStart, RsComplex* window, const unsigned int wSize, const RS_FLOAT rate,
	                      const RS_FLOAT rStart, const RsComplex* resp, const unsigned int rSize)
	{
		int start_sample = static_cast<int>(rs_portable::rsRound(rate * (rStart - wStart)));
		//Get the offset into the response
		unsigned int roffset = 0;
		if (start_sample < 0)
		{
			roffset = -start_sample;
			start_sample = 0;
		}
		//Loop through the response, adding it to the window
		for (unsigned int i = roffset; i < rSize && i + start_sample < wSize; i++)
		{
			window[i + start_sample] += resp[i];
		}
	}

	/// Generate the phase noise samples for the receive window
	RS_FLOAT* generatePhaseNoise(const Receiver* recv, const unsigned int wSize, const RS_FLOAT rate,
	                            RS_FLOAT& carrier, bool& enabled)
	{
		//Get a pointer to the receiver's timing object
		auto* timing = dynamic_cast<ClockModelTiming*>(recv->getTiming());
		if (!timing)
		{
			throw std::runtime_error("[BUG] Could not cast receiver->GetTiming() to ClockModelTiming");
		}
		//Allocate memory for the phase noise samples
		auto* noise = new RS_FLOAT[wSize];
		enabled = timing->enabled();
		if (enabled)
		{
			//Generate phase noise if timing is enabled
			for (unsigned int i = 0; i < wSize; i++)
			{
				noise[i] = timing->nextNoiseSample();
			}
			//Calculate the number of samples to skip
			if (timing->getSyncOnPulse())
			{
				timing->reset();
				const int skip = static_cast<int>(std::floor(rate * recv->getWindowSkip()));
				timing->skipSamples(skip);
			}
			else
			{
				const long skip = std::floor(rate / recv->getPrf() - rate * recv->getWindowLength());
				timing->skipSamples(skip);
			}
			carrier = timing->getFrequency();
		}
		else
		{
			// Clear samples if not
			for (unsigned int i = 0; i < wSize; i++)
			{
				noise[i] = 0;
			}
			carrier = 1;
		}
		return noise;
	}

	/// Add clock phase noise to the receive window
	void addPhaseNoiseToWindow(const RS_FLOAT* noise, RsComplex* window, const unsigned int wSize)
	{
		for (unsigned int i = 0; i < wSize; i++)
		{
			//Modify the phase
			if (std::isnan(noise[i]))
			{
				throw std::runtime_error("[BUG] Noise is NAN in AddPhaseNoiseToWindow");
			}
			const RsComplex mn = exp(RsComplex(0.0, 1.0) * noise[i]);
			window[i] *= mn;
			if (std::isnan(window[i].real()) || std::isnan(window[i].imag()))
			{
				throw std::runtime_error("[BUG] NAN encountered in AddPhaseNoiseToWindow");
			}
		}
	}

	/// Export to the FersBin file format
	void exportResponseFersBin(const std::vector<Response*>& responses, const Receiver* recv,
	                           const std::string& recvName)
	{
		//Bail if there are no responses to export
		if (responses.empty())
		{
			return;
		}

		const long int out_bin = openHdf5File(recvName);

		// Create a threaded render object, to manage the rendering process
		const ThreadedRenderer thr_renderer(&responses, recv, RsParameters::renderThreads());

		// Now loop through the responses and write them to the file
		const int window_count = recv->getWindowCount();
		for (int i = 0; i < window_count; i++)
		{
			const RS_FLOAT length = recv->getWindowLength();
			const RS_FLOAT rate = RsParameters::rate() * RsParameters::oversampleRatio();
			auto size = static_cast<unsigned int>(std::ceil(length * rate));
			// rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Length: %g Size: %d\n", length, size);
			//Generate the phase noise samples for the window
			RS_FLOAT carrier;
			bool pn_enabled;
			const RS_FLOAT* pnoise = generatePhaseNoise(recv, size, rate, carrier, pn_enabled);
			// Get the window start time, including clock drift effects
			RS_FLOAT start = recv->getWindowStart(i) + pnoise[0] / (2 * M_PI * carrier);
			// Get the fraction of a sample of the window delay
			const RS_FLOAT frac_delay = start * rate - rs_portable::rsRound(start * rate);
			start = rs_portable::rsRound(start * rate) / rate;
			// rsFloat start = recv->GetWindowStart(i);
			// Allocate memory for the entire window
			auto* window = new RsComplex[size];
			//Clear the window in memory
			memset(window, 0, sizeof(RsComplex) * size);
			//Add Noise to the window
			addNoiseToWindow(window, size, recv->getNoiseTemperature());
			// Render to the window, using the threaded renderer
			thr_renderer.renderWindow(window, length, start, frac_delay);
			//Downsample the contents of the window, if appropriate
			if (RsParameters::oversampleRatio() != 1)
			{
				// Calculate the size of the window after downsampling
				const unsigned int new_size = size / RsParameters::oversampleRatio();
				// Allocate memory for downsampled window
				auto* tmp = new RsComplex[new_size];
				//Downsample the data into tmp
				downsample(window, size, tmp, RsParameters::oversampleRatio());
				// Set tmp as the new window
				size = new_size;
				delete[] window;
				window = tmp;
			}
			//Add Phase noise to the window
			if (pn_enabled)
			{
				addPhaseNoiseToWindow(pnoise, window, size);
			}
			//Clean up the phase noise array
			delete[] pnoise;
			//Normalize and quantize the window
			const RS_FLOAT fullscale = quantizeWindow(window, size);
			//Export the binary format
			if (RsParameters::exportBinary())
			{
				rshdf5::addChunkToFile(out_bin, window, size, start, RsParameters::rate(), fullscale, i);
			}
			// Clean up memory
			delete[] window;
		}
		// Close the binary and csv files
		if (out_bin)
		{
			rshdf5::closeFile(out_bin);
		}
	}
}

/// Export the responses received by a receiver to an XML file
void rs::exportReceiverXml(const std::vector<Response*>& responses, const std::string& filename)
{
	//Create the document
	TiXmlDocument doc;
	auto decl = std::make_unique<TiXmlDeclaration>("1.0", "", "");
	doc.LinkEndChild(decl.release());
	//Create a root node for the document
	auto* root = new TiXmlElement("receiver");
	doc.LinkEndChild(root);

	//dump each response in turn
	for (auto response : responses)
	{
		response->renderXml(root);
	}

	// Write the output to the specified file
	if (!doc.SaveFile(filename + ".fersxml"))
	{
		throw std::runtime_error("Failed to save XML file: " + filename + ".fersxml");
	}
}

/// Export the responses in CSV format
void rs::exportReceiverCsv(const std::vector<Response*>& responses, const std::string& filename)
{
	std::map<std::string, std::ofstream*> streams; //map of per-transmitter open files
	for (auto response : responses)
	{
		std::ofstream* of;
		// See if a file is already open for that transmitter
		// If the file for that transmitter does not exist, add it
		if (auto ofi = streams.find(response->getTransmitterName()); ofi ==
			streams.end())
		{
			std::ostringstream oss;
			oss << filename << "_" << response->getTransmitterName() << ".csv";
			//Open a new ofstream with that name
			of = new std::ofstream(oss.str().c_str());
			of->setf(std::ios::scientific); //Set the stream in scientific notation mode
			//Check if the open succeeded
			if (!*of)
			{
				throw std::runtime_error("[ERROR] Could not open file " + oss.str() + " for writing");
			}
			//Add the file to the map
			streams[response->getTransmitterName()] = of;
		}
		else
		{
			//The file is already open
			of = ofi->second;
		}
		// Render the response to the file
		response->renderCsv(*of);
	}
	//Close all the files that we opened
	for (auto & stream : streams)
	{
		delete stream.second;
	}
}

/// Export the receiver pulses to the specified binary file, using the specified quantization
void rs::exportReceiverBinary(const std::vector<Response*>& responses, const Receiver* recv, const std::string& recvName)
{
	exportResponseFersBin(responses, recv, recvName);
}

//
// ThreadedRenderer Implementation
//

/// Constructor
ThreadedRenderer::ThreadedRenderer(const std::vector<Response*>* responses, const Receiver* recv, const int maxThreads):
	_responses(responses),
	_recv(recv),
	_max_threads(maxThreads)
{
}

/// Destructor
ThreadedRenderer::~ThreadedRenderer() = default;

/// Render all the responses in a single window
void ThreadedRenderer::renderWindow(RsComplex* window, RS_FLOAT length, RS_FLOAT start, RS_FLOAT fracDelay) const
{
	RS_FLOAT end = start + length; // End time of the window
	//Put together a list of responses seen by this window
	std::queue<Response*> work_list;
	for (auto response : *_responses)
	{
		RS_FLOAT resp_start = response->startTime();
		if (RS_FLOAT resp_end = response->endTime(); resp_start <= end && resp_end >= start)
		{
			work_list.push(response);
		}
	}
	//Manage threads with boost::thread_group
	boost::thread_group group;
	// Create a mutex to protect the worklist
	boost::mutex work_list_mutex;
	//Create a mutex to protect the window
	boost::mutex window_mutex;
	//Create the number of threads we are allowed
	{
		std::vector<std::unique_ptr<RenderThread>> threads;
		// David Young: Do not put this scoped lock outside the encapsulating braces otherwise a deadlock will occur
		boost::mutex::scoped_lock lock(work_list_mutex);
		// rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Spawning %d render threads\n", max_threads);
		for (int i = 0; i < _max_threads; i++)
		{
			// rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Spawning %d\n", i);
			auto thr = std::make_unique<RenderThread>(i, &window_mutex, window, length, start, fracDelay, &work_list_mutex, &work_list);
			group.create_thread(*thr);
			threads.push_back(std::move(thr));
		}
	}
	// Wait until all our threads are complete
	group.join_all();
}

//
// RenderThread Implementation
//

/// Constructor
RenderThread::RenderThread(const int serial, boost::mutex* windowMutex, RsComplex* window, const RS_FLOAT length,
                           const RS_FLOAT start,
                           const RS_FLOAT fracDelay, boost::mutex* workListMutex, std::queue<Response*>* workList):
	_serial(serial),
	_window_mutex(windowMutex),
	_window(window),
	_length(length),
	_start(start),
	_frac_delay(fracDelay),
	_work_list_mutex(workListMutex),
	_work_list(workList)
{
}

/// Destructor
RenderThread::~RenderThread() = default;

/// Step through the worklist, rendering the required responses
void RenderThread::operator()()
{
	const RS_FLOAT rate = RsParameters::rate() * RsParameters::oversampleRatio();
	const auto size = static_cast<unsigned int>(std::ceil(_length * rate));
	// Allocate memory for the local window
	_local_window = new RsComplex[size];
	for (unsigned int i = 0; i < size; i++)
	{
		_local_window[i] = 0;
	}
	// Loop until the work list is empty
	const Response* resp = getWork();
	while (resp)
	{
		unsigned int psize;
		RS_FLOAT prate;
		// Render the pulse into memory
		boost::shared_array<RsComplex> array = resp->renderBinary(prate, psize, _frac_delay);
		// Add the array to the window
		addWindow(array.get(), resp->startTime(), psize);
		// Get more work, if it's available
		resp = getWork();
	}
	{
		// Lock the window mutex, to ensure that we are the only thread accessing the window
		boost::mutex::scoped_lock lock(*_window_mutex);
		// Add local window to the global window
		for (unsigned int i = 0; i < size; i++)
		{
			_window[i] += _local_window[i];
		}
	}
	delete[] _local_window;
}

/// Add the array to the window, locking the window lock in advance
void RenderThread::addWindow(const RsComplex* array, const RS_FLOAT startTime, const unsigned int arraySize) const
{
	//Calculate required window parameters
	const RS_FLOAT rate = RsParameters::rate() * RsParameters::oversampleRatio();
	const auto size = static_cast<unsigned int>(std::ceil(_length * rate));
	// Add the array to the correct place in the local window
	addArrayToWindow(_start, _local_window, size, rate, startTime, array, arraySize);
}

/// Get a response from the worklist, returning NULL on failure
Response* RenderThread::getWork() const
{
	// rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Thread %d getting work\n", serial);
	Response* ret;
	{
		//Scope for scoped lock
		//Lock the work list mutex
		boost::mutex::scoped_lock lock(*_work_list_mutex);
		if (_work_list->empty()) //If the work list is empty, return NULL
		{
			return nullptr;
		}
		// Get the response at the front of the queue
		ret = _work_list->front();
		_work_list->pop();
	}
	return ret;
	//scoped_lock unlocks mutex here
}
