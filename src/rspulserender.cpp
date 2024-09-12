// rspulserender.cpp
// Performs the second phase of the simulation - rendering the result
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 7 June 2006

#define TIXML_USE_STL

#include "rspulserender.h"

#include <cstdio>
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

#include "dsp_filters.h"
#include "hdf5_export.h"
#include "noise_generators.h"
#include "parameters.h"
#include "portable_utils.h"
#include "radar_system.h"
#include "response.h"
#include "timing.h"

using namespace rs;

namespace
{
	long openHdf5File(const std::string& recvName)
	{
		long hdf5_file = 0;
		if (parameters::exportBinary())
		{
			std::ostringstream b_oss;
			b_oss.setf(std::ios::scientific);
			b_oss << recvName << ".h5";
			hdf5_file = hdf5_export::createFile(b_oss.str());
		}
		return hdf5_file;
	}

	void addNoiseToWindow(RS_COMPLEX* data, const unsigned size, const RS_FLOAT temperature)
	{
		if (temperature == 0) { return; }
		const RS_FLOAT power = rs_noise::noiseTemperatureToPower(temperature,
		                                                         parameters::rate() * parameters::oversampleRatio()
		                                                         / 2);
		WgnGenerator generator(sqrt(power) / 2.0);
		for (unsigned i = 0; i < size; i++) { data[i] += RS_COMPLEX(generator.getSample(), generator.getSample()); }
	}

	void adcSimulate(RS_COMPLEX* data, const unsigned size, const unsigned bits, const RS_FLOAT fullscale)
	{
		const RS_FLOAT levels = pow(2, bits - 1);
		for (unsigned it = 0; it < size; it++)
		{
			RS_FLOAT i = std::floor(levels * data[it].real() / fullscale) / levels;
			RS_FLOAT q = std::floor(levels * data[it].imag() / fullscale) / levels;
			// TODO: can use std::clamp
			if (i > 1) { i = 1; }
			else if (i < -1) { i = -1; }
			if (q > 1) { q = 1; }
			else if (q < -1) { q = -1; }
			data[it] = RS_COMPLEX(i, q);
		}
	}

	RS_FLOAT quantizeWindow(RS_COMPLEX* data, const unsigned size)
	{
		// TODO: This can be simplified and optimized
		RS_FLOAT max = 0;
		for (unsigned i = 0; i < size; i++)
		{
			if (std::fabs(data[i].real()) > max) { max = std::fabs(data[i].real()); }
			if (std::fabs(data[i].imag()) > max) { max = std::fabs(data[i].imag()); }
			if (std::isnan(data[i].real()) || std::isnan(data[i].imag()))
			{
				throw std::runtime_error("NaN in QuantizeWindow -- early");
			}
		}
		if (parameters::adcBits() > 0) { adcSimulate(data, size, parameters::adcBits(), max); }
		else
		{
			if (max != 0)
			{
				for (unsigned i = 0; i < size; i++)
				{
					data[i] /= max;
					if (std::isnan(data[i].real()) || std::isnan(data[i].imag()))
					{
						throw std::runtime_error("NaN in QuantizeWindow -- late");
					}
				}
			}
		}
		return max;
	}

	void addArrayToWindow(const RS_FLOAT wStart, RS_COMPLEX* window, const unsigned wSize, const RS_FLOAT rate,
	                      const RS_FLOAT rStart, const RS_COMPLEX* resp, const unsigned rSize)
	{
		int start_sample = static_cast<int>(portable_utils::rsRound(rate * (rStart - wStart)));
		unsigned roffset = 0;
		if (start_sample < 0)
		{
			roffset = -start_sample;
			start_sample = 0;
		}
		for (unsigned i = roffset; i < rSize && i + start_sample < wSize; i++) { window[i + start_sample] += resp[i]; }
	}

	RS_FLOAT* generatePhaseNoise(const Receiver* recv, const unsigned wSize, const RS_FLOAT rate,
	                             RS_FLOAT& carrier, bool& enabled)
	{
		auto* timing = dynamic_cast<ClockModelTiming*>(recv->getTiming());
		if (!timing) { throw std::runtime_error("[BUG] Could not cast receiver->GetTiming() to ClockModelTiming"); }
		auto* noise = new RS_FLOAT[wSize];
		enabled = timing->enabled();
		if (enabled)
		{
			for (unsigned i = 0; i < wSize; i++) { noise[i] = timing->nextNoiseSample(); }
			if (timing->getSyncOnPulse()) // TODO: BUG #2 - sync_on_pulse always false
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
			carrier = timing->getFrequency(); // TODO: BUG #1,#3 - carrier has no effect on phase noise?
		}
		else
		{
			// TODO: Can use std:fill
			for (unsigned i = 0; i < wSize; i++) { noise[i] = 0; }
			carrier = 1; // TODO: BUG #3 - carrier has no effect on phase noise?
		}
		return noise;
	}

	void addPhaseNoiseToWindow(const RS_FLOAT* noise, RS_COMPLEX* window, const unsigned wSize)
	{
		for (unsigned i = 0; i < wSize; i++)
		{
			if (std::isnan(noise[i])) { throw std::runtime_error("[BUG] Noise is NaN in addPhaseNoiseToWindow"); }
			const RS_COMPLEX mn = exp(RS_COMPLEX(0.0, 1.0) * noise[i]);
			window[i] *= mn;
			if (std::isnan(window[i].real()) || std::isnan(window[i].imag()))
			{
				throw std::runtime_error("[BUG] NaN encountered in addPhaseNoiseToWindow");
			}
		}
	}

	void exportResponseFersBin(const std::vector<Response*>& responses, const Receiver* recv,
	                           const std::string& recvName)
	{
		// TODO: This can be simplified and optimized
		//Bail if there are no responses to export
		if (responses.empty()) { return; }

		const long out_bin = openHdf5File(recvName);

		// Create a threaded render object, to manage the rendering process
		const ThreadedRenderer thr_renderer(&responses, recv, parameters::renderThreads());

		// Now loop through the responses and write them to the file
		const int window_count = recv->getWindowCount();
		for (int i = 0; i < window_count; i++)
		{
			const RS_FLOAT length = recv->getWindowLength();
			const RS_FLOAT rate = parameters::rate() * parameters::oversampleRatio();
			auto size = static_cast<unsigned>(std::ceil(length * rate));
			// logging::printf(logging::RS_VERY_VERBOSE, "Length: %g Size: %d\n", length, size);
			//Generate the phase noise samples for the window
			RS_FLOAT carrier;
			bool pn_enabled;
			const RS_FLOAT* pnoise = generatePhaseNoise(recv, size, rate, carrier, pn_enabled);
			// Get the window start time, including clock drift effects
			RS_FLOAT start = recv->getWindowStart(i) + pnoise[0] / (2 * M_PI * carrier);
			// Get the fraction of a sample of the window delay
			const RS_FLOAT frac_delay = start * rate - portable_utils::rsRound(start * rate);
			start = portable_utils::rsRound(start * rate) / rate;
			// rsFloat start = recv->GetWindowStart(i);
			// Allocate memory for the entire window
			auto* window = new RS_COMPLEX[size];
			//Clear the window in memory
			memset(window, 0, sizeof(RS_COMPLEX) * size);
			//Add Noise to the window
			addNoiseToWindow(window, size, recv->getNoiseTemperature());
			// Render to the window, using the threaded renderer
			thr_renderer.renderWindow(window, length, start, frac_delay);
			//Downsample the contents of the window, if appropriate
			if (parameters::oversampleRatio() != 1)
			{
				// Calculate the size of the window after downsampling
				const unsigned new_size = size / parameters::oversampleRatio();
				// Allocate memory for downsampled window
				auto* tmp = new RS_COMPLEX[new_size];
				//Downsample the data into tmp
				downsample(window, size, tmp, parameters::oversampleRatio());
				// Set tmp as the new window
				size = new_size;
				delete[] window;
				window = tmp;
			}
			//Add Phase noise to the window
			if (pn_enabled) { addPhaseNoiseToWindow(pnoise, window, size); }
			//Clean up the phase noise array
			delete[] pnoise;
			//Normalize and quantize the window
			const RS_FLOAT fullscale = quantizeWindow(window, size);
			//Export the binary format
			if (parameters::exportBinary())
			{
				hdf5_export::addChunkToFile(out_bin, window, size, start, parameters::rate(), fullscale, i);
			}
			// Clean up memory
			delete[] window;
		}
		// Close the binary and csv files
		if (out_bin) { hdf5_export::closeFile(out_bin); }
	}
}

void rs::exportReceiverXml(const std::vector<Response*>& responses, const std::string& filename)
{
	TiXmlDocument doc;
	auto decl = std::make_unique<TiXmlDeclaration>("1.0", "", "");
	doc.LinkEndChild(decl.release());
	auto* root = new TiXmlElement("receiver");
	doc.LinkEndChild(root);
	for (const auto response : responses) { response->renderXml(root); }
	if (!doc.SaveFile(filename + ".fersxml"))
	{
		throw std::runtime_error("Failed to save XML file: " + filename + ".fersxml");
	}
}

void rs::exportReceiverCsv(const std::vector<Response*>& responses, const std::string& filename)
{
	std::map<std::string, std::ofstream*> streams;
	for (const auto response : responses)
	{
		std::ofstream* of;
		if (auto ofi = streams.find(response->getTransmitterName()); ofi == streams.end())
		{
			std::ostringstream oss;
			oss << filename << "_" << response->getTransmitterName() << ".csv";
			of = new std::ofstream(oss.str().c_str());
			of->setf(std::ios::scientific);
			if (!*of) { throw std::runtime_error("[ERROR] Could not open file " + oss.str() + " for writing"); }
			streams[response->getTransmitterName()] = of;
		}
		else { of = ofi->second; }
		response->renderCsv(*of);
	}
	for (auto& [fst, snd] : streams) { delete snd; }
}

void rs::exportReceiverBinary(const std::vector<Response*>& responses, const Receiver* recv,
                              const std::string& recvName) { exportResponseFersBin(responses, recv, recvName); }

//
// ThreadedRenderer Implementation
//

void ThreadedRenderer::renderWindow(RS_COMPLEX* window, RS_FLOAT length, RS_FLOAT start, RS_FLOAT fracDelay) const
{
	RS_FLOAT end = start + length;
	std::queue<Response*> work_list;
	for (auto response : *_responses)
	{
		RS_FLOAT resp_start = response->startTime();
		if (RS_FLOAT resp_end = response->endTime(); resp_start <= end && resp_end >= start)
		{
			work_list.push(response);
		}
	}
	boost::thread_group group;
	boost::mutex work_list_mutex, window_mutex;
	{
		std::vector<std::unique_ptr<RenderThread>> threads;
		boost::mutex::scoped_lock lock(work_list_mutex);
		for (int i = 0; i < _max_threads; i++)
		{
			auto thr = std::make_unique<RenderThread>(i, &window_mutex, window, length, start, fracDelay,
			                                          &work_list_mutex, &work_list);
			group.create_thread(*thr);
			threads.push_back(std::move(thr));
		}
	}
	group.join_all();
}

//
// RenderThread Implementation
//

void RenderThread::operator()()
{
	const RS_FLOAT rate = parameters::rate() * parameters::oversampleRatio();
	const auto size = static_cast<unsigned>(std::ceil(_length * rate));
	_local_window = new RS_COMPLEX[size]();
	const Response* resp = getWork();
	while (resp)
	{
		unsigned psize;
		RS_FLOAT prate;
		std::shared_ptr<RS_COMPLEX[]> array = resp->renderBinary(prate, psize, _frac_delay);
		addWindow(array.get(), resp->startTime(), psize);
		resp = getWork();
	}
	{
		boost::mutex::scoped_lock lock(*_window_mutex);
		for (unsigned i = 0; i < size; i++) { _window[i] += _local_window[i]; }
	}
	delete[] _local_window;
}

void RenderThread::addWindow(const RS_COMPLEX* array, const RS_FLOAT startTime, const unsigned arraySize) const
{
	const RS_FLOAT rate = parameters::rate() * parameters::oversampleRatio();
	const auto size = static_cast<unsigned>(std::ceil(_length * rate));
	addArrayToWindow(_start, _local_window, size, rate, startTime, array, arraySize);
}

Response* RenderThread::getWork() const
{
	Response* ret;
	{
		boost::mutex::scoped_lock lock(*_work_list_mutex);
		if (_work_list->empty()) { return nullptr; }
		ret = _work_list->front();
		_work_list->pop();
	}
	return ret;
}
