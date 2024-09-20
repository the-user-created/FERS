// received_export.cpp
// Export receiver data to various formats
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "receiver_export.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <tinyxml.h>

#include "hdf5_export.h"
#include "response_renderer.h"
#include "core/parameters.h"
#include "core/portable_utils.h"
#include "noise/noise_utils.h"
#include "radar/radar_system.h"

namespace
{
	long openHdf5File(const std::string& recvName)
	{
		long hdf5_file = 0;
		if (parameters::exportBinary())
		{
			std::ostringstream b_oss;
			b_oss << std::scientific << recvName << ".h5";
			hdf5_file = hdf5_export::createFile(b_oss.str());
		}
		return hdf5_file;
	}

	void addNoiseToWindow(RS_COMPLEX* data, const unsigned size, const RS_FLOAT temperature)
	{
		if (temperature == 0) { return; }

		const RS_FLOAT power = noise_utils::noiseTemperatureToPower(
			temperature,
			parameters::rate() * parameters::oversampleRatio() / 2
		);

		rs::WgnGenerator generator(std::sqrt(power) / 2.0);

		for (unsigned i = 0; i < size; ++i)
		{
			const RS_COMPLEX noise(generator.getSample(), generator.getSample());
			data[i] += noise;
		}
	}

	void adcSimulate(RS_COMPLEX* data, const unsigned size, const unsigned bits, RS_FLOAT fullscale)
	{
		const RS_FLOAT levels = std::pow(2, bits - 1);

		for (unsigned it = 0; it < size; ++it)
		{
			// Normalize and quantize the real and imaginary parts
			RS_FLOAT i = std::floor(levels * data[it].real() / fullscale) / levels;
			RS_FLOAT q = std::floor(levels * data[it].imag() / fullscale) / levels;

			// Use std::clamp to ensure the values are within the range [-1, 1]
			i = std::clamp(i, -1.0, 1.0);
			q = std::clamp(q, -1.0, 1.0);

			data[it] = RS_COMPLEX(i, q);
		}
	}

	RS_FLOAT quantizeWindow(RS_COMPLEX* data, const unsigned size)
	{
		// Find the maximum absolute value across real and imaginary parts
		RS_FLOAT max_value = 0;

		// Loop through the data to calculate the maximum and check for NaNs
		for (unsigned i = 0; i < size; ++i)
		{
			RS_FLOAT real_abs = std::fabs(data[i].real());
			RS_FLOAT imag_abs = std::fabs(data[i].imag());

			max_value = std::max({max_value, real_abs, imag_abs});

			// Check for NaN in both real and imaginary parts
			if (std::isnan(real_abs) || std::isnan(imag_abs))
			{
				throw std::runtime_error("NaN encountered in QuantizeWindow -- early");
			}
		}

		// Simulate ADC if adcBits parameter is greater than 0
		if (parameters::adcBits() > 0) { adcSimulate(data, size, parameters::adcBits(), max_value); }
		else if (max_value != 0)
		{
			// Normalize the data if max_value is not zero
			for (unsigned i = 0; i < size; ++i)
			{
				data[i] /= max_value;

				// Re-check for NaN after normalization
				if (std::isnan(data[i].real()) || std::isnan(data[i].imag()))
				{
					throw std::runtime_error("NaN encountered in QuantizeWindow -- late");
				}
			}
		}

		return max_value;
	}

	RS_FLOAT* generatePhaseNoise(const rs::Receiver* recv, const unsigned wSize, const RS_FLOAT rate,
	                             RS_FLOAT& carrier, bool& enabled)
	{
		// Get the timing model from the receiver
		const auto timing = recv->getTiming();
		if (!timing) { throw std::runtime_error("[BUG] Could not cast receiver->GetTiming() to ClockModelTiming"); }

		// Use a smart pointer for memory safety; can be released later if raw pointer is required
		auto noise = std::make_unique<RS_FLOAT[]>(wSize);

		enabled = timing->enabled();

		if (enabled)
		{
			for (unsigned i = 0; i < wSize; ++i) { noise[i] = timing->nextNoiseSample(); }

			if (timing->getSyncOnPulse())
			{
				timing->reset();
				const int skip = static_cast<int>(std::floor(rate * recv->getWindowSkip()));
				timing->skipSamples(skip);
			}
			else
			{
				const long skip = static_cast<long>(std::floor(
					rate / recv->getWindowPrf() - rate * recv->getWindowLength()));
				timing->skipSamples(skip);
			}

			carrier = timing->getFrequency();
		}
		else
		{
			// Use std::fill for setting noise to 0
			std::fill_n(noise.get(), wSize, 0);
			carrier = 1;
		}

		// Release the smart pointer to return the raw pointer, maintaining original behavior
		return noise.release();
	}

	void addPhaseNoiseToWindow(const RS_FLOAT* noise, RS_COMPLEX* window, const unsigned wSize)
	{
		for (unsigned i = 0; i < wSize; ++i)
		{
			if (std::isnan(noise[i])) { throw std::runtime_error("[BUG] Noise is NaN in addPhaseNoiseToWindow"); }

			// Generate the phase noise multiplier using std::polar for efficiency and clarity
			const RS_COMPLEX phase_noise = std::polar(1.0, noise[i]);

			// Apply the phase noise to the window element
			window[i] *= phase_noise;

			// Check for NaN in the result
			if (std::isnan(window[i].real()) || std::isnan(window[i].imag()))
			{
				throw std::runtime_error("[BUG] NaN encountered in addPhaseNoiseToWindow");
			}
		}
	}

	void exportResponseFersBin(const std::vector<rs::Response*>& responses, const rs::Receiver* recv,
	                           const std::string& recvName)
	{
		// Bail if there are no responses to export
		if (responses.empty()) { return; }

		// Open HDF5 file for writing
		const long out_bin = openHdf5File(recvName);

		// Create a threaded render object to manage the rendering process
		const response_renderer::ThreadedResponseRenderer thr_renderer(&responses, recv, parameters::renderThreads());

		// Retrieve the window count from the receiver
		const int window_count = recv->getWindowCount();

		// Loop through each window
		for (int i = 0; i < window_count; ++i)
		{
			const RS_FLOAT length = recv->getWindowLength();
			const RS_FLOAT rate = parameters::rate() * parameters::oversampleRatio();
			auto size = static_cast<unsigned>(std::ceil(length * rate));

			// Generate phase noise samples for the window
			RS_FLOAT carrier;
			bool pn_enabled;
			std::unique_ptr<const RS_FLOAT[]> pnoise(generatePhaseNoise(recv, size, rate, carrier, pn_enabled));

			// Get the window start time, including clock drift effects
			RS_FLOAT start = recv->getWindowStart(i) + pnoise[0] / (2 * M_PI * carrier);

			// Calculate the fractional delay
			const RS_FLOAT frac_delay = start * rate - std::round(start * rate);
			start = std::round(start * rate) / rate;

			// Allocate memory for the window using smart pointers
			std::unique_ptr<RS_COMPLEX[]> window = std::make_unique<RS_COMPLEX[]>(size);

			// Clear the window memory
			std::fill_n(window.get(), size, RS_COMPLEX(0.0, 0.0));

			// Add noise to the window
			addNoiseToWindow(window.get(), size, recv->getNoiseTemperature());

			// Render the window using the threaded renderer
			thr_renderer.renderWindow(window.get(), length, start, frac_delay);

			// Downsample the window if the oversample ratio is greater than 1
			if (parameters::oversampleRatio() != 1)
			{
				// Calculate the new size after downsampling
				const unsigned new_size = size / parameters::oversampleRatio();

				// Allocate memory for the downsampled window
				std::unique_ptr<RS_COMPLEX[]> tmp = std::make_unique<RS_COMPLEX[]>(new_size);

				// Perform downsampling
				rs::downsample(window.get(), size, tmp.get(), parameters::oversampleRatio());

				// Replace the window with the downsampled one
				size = new_size;
				window = std::move(tmp);
			}

			// Add phase noise to the window if enabled
			if (pn_enabled) { addPhaseNoiseToWindow(pnoise.get(), window.get(), size); }

			// Normalize and quantize the window
			const RS_FLOAT fullscale = quantizeWindow(window.get(), size);

			// Export the binary format if enabled
			if (parameters::exportBinary())
			{
				hdf5_export::addChunkToFile(out_bin, window.get(), size, start, parameters::rate(), fullscale, i);
			}
		}

		// Close the binary file
		if (out_bin) { hdf5_export::closeFile(out_bin); }
	}
}

namespace receiver_export
{
	void exportReceiverXml(const std::vector<rs::Response*>& responses, const std::string& filename)
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

	void exportReceiverCsv(const std::vector<rs::Response*>& responses, const std::string& filename)
	{
		// Use std::unique_ptr to handle automatic cleanup of the streams
		std::map<std::string, std::unique_ptr<std::ofstream>> streams;

		// Iterate over each response in the vector
		for (const auto* response : responses)
		{
			// Get the transmitter name from the response
			const std::string& transmitter_name = response->getTransmitterName();

			// Check if a stream for this transmitter already exists
			if (auto it = streams.find(transmitter_name); it == streams.end())
			{
				// If not found, create a new output file stream
				std::ostringstream oss;
				oss << filename << "_" << transmitter_name << ".csv";

				auto of = std::make_unique<std::ofstream>(oss.str());
				of->setf(std::ios::scientific);

				if (!*of) { throw std::runtime_error("[ERROR] Could not open file " + oss.str() + " for writing"); }

				// Add the new stream to the map
				streams[transmitter_name] = std::move(of);
			}

			// Render the CSV for the current response
			response->renderCsv(*streams[transmitter_name]);
		}
	}

	void exportReceiverBinary(const std::vector<rs::Response*>& responses, const rs::Receiver* recv,
	                          const std::string& recvName) { exportResponseFersBin(responses, recv, recvName); }
}
