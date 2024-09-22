// received_export.cpp
// Export receiver data to various formats
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "receiver_export.h"

#include <algorithm>                        // for clamp, max, __fill_fn
#include <cmath>                            // for isnan, floor, fabs, round
#include <complex>                          // for complex, polar
#include <filesystem>                       // for path
#include <format>                           // for format
#include <fstream>                          // for basic_ofstream, basic_ios
#include <map>                              // for map, _Rb_tree_iterator
#include <numbers>                          // for pi
#include <optional>                         // for optional
#include <ranges>                           // for _Zip, views, zip, zip_view
#include <stdexcept>                        // for runtime_error
#include <tinyxml.h>                        // for TiXmlDeclaration, TiXmlDo...
#include <tuple>                            // for tuple
#include <utility>                          // for move, pair
#include <vector>                           // for vector

#include "config.h"                         // for RealType, ComplexType
#include "hdf5_handler.h"                   // for addChunkToFile, closeFile
#include "response_renderer.h"              // for ThreadedResponseRenderer
#include "core/parameters.h"                // for oversampleRatio, rate
#include "noise/noise_generators.h"         // for WgnGenerator
#include "noise/noise_utils.h"              // for noiseTemperatureToPower
#include "radar/radar_system.h"             // for Receiver
#include "serialization/response.h"         // for Response
#include "signal_processing/dsp_filters.h"  // for downsample
#include "timing/timing.h"                  // for Timing

namespace fs = std::filesystem;

namespace
{
	long openHdf5File(const std::string& recvName)
	{
		if (!params::exportBinary()) { return 0; }

		const auto hdf5_filename = std::format("{}.h5", recvName);
		return serial::createFile(hdf5_filename);
	}

	void addNoiseToWindow(std::span<ComplexType> data, const RealType temperature)
	{
		if (temperature == 0) { return; }

		const RealType power = noise::noiseTemperatureToPower(
			temperature,
			params::rate() * params::oversampleRatio() / 2
		);

		noise::WgnGenerator generator(std::sqrt(power) / 2.0);

		for (auto& sample : data)
		{
			const ComplexType noise_sample(generator.getSample(), generator.getSample());
			sample += noise_sample;
		}
	}

	void adcSimulate(std::span<ComplexType> data, const unsigned bits, RealType fullscale)
	{
		const RealType levels = std::pow(2, bits - 1);

		for (auto& sample : data)
		{
			auto [i, q] = std::tuple{
				std::clamp(std::floor(levels * sample.real() / fullscale) / levels, -1.0, 1.0),
				std::clamp(std::floor(levels * sample.imag() / fullscale) / levels, -1.0, 1.0)
			};

			sample = ComplexType(i, q);
		}
	}

	RealType quantizeWindow(std::span<ComplexType> data)
	{
		// Find the maximum absolute value across real and imaginary parts
		RealType max_value = 0;

		for (const auto& sample : data)
		{
			const RealType real_abs = std::fabs(sample.real());
			const RealType imag_abs = std::fabs(sample.imag());

			max_value = std::max({max_value, real_abs, imag_abs});

			// Check for NaN in both real and imaginary parts
			if (std::isnan(real_abs) || std::isnan(imag_abs))
			{
				throw std::runtime_error("NaN encountered in QuantizeWindow -- early");
			}
		}

		// Simulate ADC if adcBits parameter is greater than 0
		if (const auto adc_bits = params::adcBits(); adc_bits > 0) { adcSimulate(data, adc_bits, max_value); }
		else if (max_value != 0)
		{
			for (auto& sample : data)
			{
				sample /= max_value;

				// Re-check for NaN after normalization
				if (std::isnan(sample.real()) || std::isnan(sample.imag()))
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
		const auto timing = recv->getTiming();
		if (!timing) { throw std::runtime_error("[BUG] Could not cast receiver->GetTiming() to ClockModelTiming"); }

		std::vector<RealType> noise(wSize);
		enabled = timing->isEnabled();

		if (enabled)
		{
			std::ranges::generate(noise, [&] { return timing->getNextSample(); });

			if (timing->getSyncOnPulse())
			{
				timing->reset();
				timing->skipSamples(static_cast<int>(std::floor(rate * recv->getWindowSkip())));
			}
			else
			{
				timing->skipSamples(static_cast<long>(std::floor(
					rate / recv->getWindowPrf() - rate * recv->getWindowLength())));
			}

			carrier = timing->getFrequency();
		}
		else
		{
			std::ranges::fill(noise, 0);
			carrier = 1;
		}

		return noise;
	}

	void addPhaseNoiseToWindow(std::span<const RealType> noise, std::span<ComplexType> window)
	{
		for (auto [n, w] : std::views::zip(noise, window))
		{
			if (std::isnan(n)) { throw std::runtime_error("[BUG] Noise is NaN in addPhaseNoiseToWindow"); }

			const ComplexType phase_noise = std::polar(1.0, n);
			w *= phase_noise;

			if (std::isnan(w.real()) || std::isnan(w.imag()))
			{
				throw std::runtime_error("[BUG] NaN encountered in addPhaseNoiseToWindow");
			}
		}
	}
}

namespace serial
{
	void exportReceiverXml(std::span<const std::unique_ptr<Response>> responses, const std::string& filename)
	{
		TiXmlDocument doc;

		// Use a smart pointer directly with release later for better ownership management
		auto decl = std::make_unique<TiXmlDeclaration>("1.0", "", "");
		doc.LinkEndChild(decl.release());

		// No smart pointer because TiXml handles the memory
		auto* root = new TiXmlElement("receiver");
		doc.LinkEndChild(root);

		// Modern range-based for loop
		for (const auto& response : responses) { response->renderXml(root); }

		// Using std::filesystem to work with the path
		if (const fs::path file_path = fs::path(filename).replace_extension(".fersxml"); !doc.
			SaveFile(file_path.string()))
		{
			throw std::runtime_error("Failed to save XML file: " + file_path.string());
		}
	}

	void exportReceiverCsv(const std::span<const std::unique_ptr<Response>> responses, const std::string& filename)
	{
		std::map<std::string, std::unique_ptr<std::ofstream>> streams;

		// Iterate over each response in the vector, using structured bindings and ranges
		for (const auto& response : responses)
		{
			const std::string& transmitter_name = response->getTransmitterName();

			// Use `std::map::try_emplace` to simplify checking and insertion
			auto [it, inserted] = streams.try_emplace(transmitter_name, nullptr);

			// If the stream for this transmitter is being inserted, open a new file
			if (inserted)
			{
				fs::path file_path = fs::path(filename).concat("_").concat(transmitter_name).replace_extension(".csv");

				auto of = std::make_unique<std::ofstream>(file_path.string());
				of->setf(std::ios::scientific);

				if (!*of) { throw std::runtime_error("Could not open file " + file_path.string() + " for writing"); }

				// Assign the opened file stream to the map entry
				it->second = std::move(of);
			}

			// Render the CSV for the current response
			response->renderCsv(*it->second);
		}
	}

	void exportReceiverBinary(const std::span<const std::unique_ptr<Response>> responses, const radar::Receiver* recv,
	                          const std::string& recvName)
	{
		// Bail if there are no responses to export
		if (responses.empty()) { return; }

		// Open HDF5 file for writing, using optional to handle a file open/close more safely
		const std::optional out_bin = openHdf5File(recvName);

		// Create a threaded render object to manage the rendering process
		const ThreadedResponseRenderer thr_renderer(responses, recv, params::renderThreads());

		// Retrieve the window count from the receiver
		const int window_count = recv->getWindowCount();

		// Loop through each window, using a more modern range-based for loop when possible
		for (int i = 0; i < window_count; ++i)
		{
			const RealType length = recv->getWindowLength();
			const RealType rate = params::rate() * params::oversampleRatio();
			auto size = static_cast<unsigned>(std::ceil(length * rate));

			// Generate phase noise samples for the window
			RealType carrier;
			bool pn_enabled;
			auto pnoise = generatePhaseNoise(recv, size, rate, carrier, pn_enabled);

			// Get the window start time, including clock drift effects
			RealType start = recv->getWindowStart(i) + pnoise[0] / (2 * std::numbers::pi * carrier);

			// Calculate the fractional delay using structured bindings for clarity
			const auto [round_start, frac_delay] = [&start, rate]
			{
				RealType rounded_start = std::round(start * rate) / rate;
				RealType fractional_delay = start * rate - std::round(start * rate);
				return std::tuple{rounded_start, fractional_delay};
			}();
			start = round_start;

			// Allocate memory for the window using std::vector with default initialization
			std::vector<ComplexType> window(size);

			// Add noise to the window
			addNoiseToWindow(window, recv->getNoiseTemperature());

			// Render the window using the threaded renderer
			thr_renderer.renderWindow(window, length, start, frac_delay);

			// Downsample the window if the oversample ratio is greater than 1
			if (params::oversampleRatio() > 1)
			{
				// Calculate the new size after downsampling
				const unsigned new_size = size / params::oversampleRatio();

				// Perform downsampling directly in place if possible, otherwise move to a new vector
				std::vector<ComplexType> tmp(new_size);
				signal::downsample(window, tmp, params::oversampleRatio());

				// Replace the window with the downsampled one
				size = new_size;
				window = std::move(tmp);
			}

			// Add phase noise to the window if enabled
			if (pn_enabled) { addPhaseNoiseToWindow(pnoise, window); }

			// Normalize and quantize the window
			const RealType fullscale = quantizeWindow(window);

			// Export the binary format if enabled, using the HDF5 file handle if it exists
			if (params::exportBinary() && out_bin)
			{
				serial::addChunkToFile(*out_bin, window, size, start, params::rate(), fullscale, i);
			}
		}

		// Close the binary file if it was opened
		if (out_bin) { closeFile(*out_bin); }
	}
}
