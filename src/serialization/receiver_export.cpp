// received_export.cpp
// Export receiver data to various formats
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "receiver_export.h"

#include <algorithm>
#include <map>
#include <tinyxml.h>

#include "hdf5_handler.h"
#include "response_renderer.h"
#include "core/parameters.h"
#include "noise/noise_utils.h"
#include "radar/radar_system.h"
#include "timing/timing.h"

namespace
{
	long openHdf5File(const std::string& recvName)
	{
		long hdf5_file = 0;
		if (params::exportBinary())
		{
			std::ostringstream b_oss;
			b_oss << std::scientific << recvName << ".h5";
			hdf5_file = serial::createFile(b_oss.str());
		}
		return hdf5_file;
	}

	void addNoiseToWindow(std::vector<ComplexType>& data, const unsigned size, const RealType temperature)
	{
		if (temperature == 0) { return; }

		const RealType power = noise::noiseTemperatureToPower(
			temperature,
			params::rate() * params::oversampleRatio() / 2
		);

		noise::WgnGenerator generator(std::sqrt(power) / 2.0);

		for (unsigned i = 0; i < size; ++i)
		{
			const ComplexType noise(generator.getSample(), generator.getSample());
			data[i] += noise;
		}
	}

	void adcSimulate(std::vector<ComplexType>& data, const unsigned size, const unsigned bits, RealType fullscale)
	{
		const RealType levels = std::pow(2, bits - 1);

		for (unsigned it = 0; it < size; ++it)
		{
			// Normalize and quantize the real and imaginary parts
			RealType i = std::floor(levels * data[it].real() / fullscale) / levels;
			RealType q = std::floor(levels * data[it].imag() / fullscale) / levels;

			// Use std::clamp to ensure the values are within the range [-1, 1]
			i = std::clamp(i, -1.0, 1.0);
			q = std::clamp(q, -1.0, 1.0);

			data[it] = ComplexType(i, q);
		}
	}

	RealType quantizeWindow(std::vector<ComplexType>& data, const unsigned size)
	{
		// Find the maximum absolute value across real and imaginary parts
		RealType max_value = 0;

		// Loop through the data to calculate the maximum and check for NaNs
		for (unsigned i = 0; i < size; ++i)
		{
			RealType real_abs = std::fabs(data[i].real());
			RealType imag_abs = std::fabs(data[i].imag());

			max_value = std::max({max_value, real_abs, imag_abs});

			// Check for NaN in both real and imaginary parts
			if (std::isnan(real_abs) || std::isnan(imag_abs))
			{
				throw std::runtime_error("NaN encountered in QuantizeWindow -- early");
			}
		}

		// Simulate ADC if adcBits parameter is greater than 0
		if (params::adcBits() > 0) { adcSimulate(data, size, params::adcBits(), max_value); }
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

	std::vector<RealType> generatePhaseNoise(const radar::Receiver* recv, const unsigned wSize, const RealType rate,
	                                         RealType& carrier, bool& enabled)
	{
		// Get the timing model from the receiver
		const auto timing = recv->getTiming();
		if (!timing) { throw std::runtime_error("[BUG] Could not cast receiver->GetTiming() to ClockModelTiming"); }

		// Use a smart pointer for memory safety; can be released later if raw pointer is required
		auto noise = std::vector<RealType>(wSize);

		enabled = timing->isEnabled();

		if (enabled)
		{
			for (unsigned i = 0; i < wSize; ++i) { noise[i] = timing->getNextSample(); }

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
			std::fill_n(noise.data(), wSize, 0);
			carrier = 1;
		}

		return noise;
	}

	void addPhaseNoiseToWindow(const std::vector<RealType>& noise, std::vector<ComplexType>& window,
	                           const unsigned wSize)
	{
		for (unsigned i = 0; i < wSize; ++i)
		{
			if (std::isnan(noise[i])) { throw std::runtime_error("[BUG] Noise is NaN in addPhaseNoiseToWindow"); }

			// Generate the phase noise multiplier using std::polar for efficiency and clarity
			const ComplexType phase_noise = std::polar(1.0, noise[i]);

			// Apply the phase noise to the window element
			window[i] *= phase_noise;

			// Check for NaN in the result
			if (std::isnan(window[i].real()) || std::isnan(window[i].imag()))
			{
				throw std::runtime_error("[BUG] NaN encountered in addPhaseNoiseToWindow");
			}
		}
	}

	void exportResponseFersBin(const std::vector<std::unique_ptr<serial::Response>>& responses,
	                           const radar::Receiver* recv,
	                           const std::string& recvName)
	{
		// Bail if there are no responses to export
		if (responses.empty()) { return; }

		// Open HDF5 file for writing
		const long out_bin = openHdf5File(recvName);

		// Create a threaded render object to manage the rendering process
		const serial::ThreadedResponseRenderer thr_renderer(responses, recv, params::renderThreads());

		// Retrieve the window count from the receiver
		const int window_count = recv->getWindowCount();

		// Loop through each window
		for (int i = 0; i < window_count; ++i)
		{
			const RealType length = recv->getWindowLength();
			const RealType rate = params::rate() * params::oversampleRatio();
			auto size = static_cast<unsigned>(std::ceil(length * rate));

			// Generate phase noise samples for the window
			RealType carrier;
			bool pn_enabled;
			std::vector<RealType> pnoise(generatePhaseNoise(recv, size, rate, carrier, pn_enabled));

			// Get the window start time, including clock drift effects
			RealType start = recv->getWindowStart(i) + pnoise[0] / (2 * PI * carrier);

			// Calculate the fractional delay
			const RealType frac_delay = start * rate - std::round(start * rate);
			start = std::round(start * rate) / rate;

			// Allocate memory for the window using smart pointers
			std::vector<ComplexType> window = std::vector<ComplexType>(size);

			// Add noise to the window
			addNoiseToWindow(window, size, recv->getNoiseTemperature());

			// Render the window using the threaded renderer
			thr_renderer.renderWindow(window, length, start, frac_delay);

			// Downsample the window if the oversample ratio is greater than 1
			if (params::oversampleRatio() != 1)
			{
				// Calculate the new size after downsampling
				const unsigned new_size = size / params::oversampleRatio();

				// Allocate memory for the downsampled window
				auto tmp = std::vector<ComplexType>(new_size);

				// Perform downsampling
				signal::downsample(window, tmp, params::oversampleRatio());

				// Replace the window with the downsampled one
				size = new_size;
				window = tmp;
			}

			// Add phase noise to the window if enabled
			if (pn_enabled) { addPhaseNoiseToWindow(pnoise, window, size); }

			// Normalize and quantize the window
			const RealType fullscale = quantizeWindow(window, size);

			// Export the binary format if enabled
			if (params::exportBinary())
			{
				serial::addChunkToFile(out_bin, window, size, start, params::rate(), fullscale, i);
			}
		}

		// Close the binary file
		if (out_bin) { serial::closeFile(out_bin); }
	}
}

namespace serial
{
	void exportReceiverXml(const std::vector<std::unique_ptr<Response>>& responses, const std::string& filename)
	{
		TiXmlDocument doc;
		auto decl = std::make_unique<TiXmlDeclaration>("1.0", "", "");
		doc.LinkEndChild(decl.release());
		// TODO: Can't make this a smart pointer...
		auto* root = new TiXmlElement("receiver");
		doc.LinkEndChild(root);
		for (const auto& response : responses) { response->renderXml(root); }
		if (!doc.SaveFile(filename + ".fersxml"))
		{
			throw std::runtime_error("Failed to save XML file: " + filename + ".fersxml");
		}
	}

	void exportReceiverCsv(const std::vector<std::unique_ptr<Response>>& responses, const std::string& filename)
	{
		// Use std::unique_ptr to handle automatic cleanup of the streams
		std::map<std::string, std::unique_ptr<std::ofstream>> streams;

		// Iterate over each response in the vector
		for (const auto& response : responses)
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

				if (!*of) { throw std::runtime_error("Could not open file " + oss.str() + " for writing"); }

				// Add the new stream to the map
				streams[transmitter_name] = std::move(of);
			}

			// Render the CSV for the current response
			response->renderCsv(*streams[transmitter_name]);
		}
	}

	void exportReceiverBinary(const std::vector<std::unique_ptr<Response>>& responses, const radar::Receiver* recv,
	                          const std::string& recvName) { exportResponseFersBin(responses, recv, recvName); }
}
