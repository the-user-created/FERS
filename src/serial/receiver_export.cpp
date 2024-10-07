// received_export.cpp
// Export receiver data to various formats
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "receiver_export.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <numbers>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>
#include <highfive/H5File.hpp>
#include <libxml/parser.h>

#include "config.h"
#include "hdf5_handler.h"
#include "libxml_wrapper.h"
#include "response_renderer.h"
#include "core/parameters.h"
#include "highfive/H5Exception.hpp"
#include "libxml/xmlstring.h"
#include "noise/noise_generators.h"
#include "noise/noise_utils.h"
#include "radar/radar_system.h"
#include "serial/response.h"
#include "signal/dsp_filters.h"
#include "timing/timing.h"

namespace fs = std::filesystem;

namespace
{
	std::optional<HighFive::File> openHdf5File(const std::string& recvName)
	{
		if (!params::exportBinary()) { return std::nullopt; }

		const auto hdf5_filename = std::format("{}.h5", recvName);

		try
		{
			// HighFive::File::Truncate will overwrite the file if it already exists
			HighFive::File file(hdf5_filename, HighFive::File::Overwrite);
			return file;
		}
		catch (const HighFive::Exception& err)
		{
			throw std::runtime_error("Error opening HDF5 file: " + std::string(err.what()));
		}
	}

	void addNoiseToWindow(std::span<ComplexType> data, const RealType temperature) noexcept
	{
		if (temperature == 0) { return; }

		const RealType power = noise::noiseTemperatureToPower(
			temperature, params::rate() * params::oversampleRatio() / 2
		);

		noise::WgnGenerator generator(std::sqrt(power) / 2.0);

		for (auto& sample : data)
		{
			const ComplexType noise_sample(generator.getSample(), generator.getSample());
			sample += noise_sample;
		}
	}

	void adcSimulate(std::span<ComplexType> data, const unsigned bits, RealType fullscale) noexcept
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
				LOG(logging::Level::FATAL, "NaN encountered in QuantizeWindow -- early");
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
					LOG(logging::Level::FATAL, "NaN encountered in QuantizeWindow -- late");
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
		if (!timing)
		{
			LOG(logging::Level::FATAL, "Could not cast receiver->GetTiming() to ClockModelTiming");
			throw std::runtime_error("Could not cast receiver->GetTiming() to ClockModelTiming");
		}

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
			if (std::isnan(n))
			{
				LOG(logging::Level::FATAL, "Noise is NaN in addPhaseNoiseToWindow");
				throw std::runtime_error("Noise is NaN in addPhaseNoiseToWindow");
			}

			const ComplexType phase_noise = std::polar(1.0, n);
			w *= phase_noise;

			if (std::isnan(w.real()) || std::isnan(w.imag()))
			{
				LOG(logging::Level::FATAL, "NaN encountered in addPhaseNoiseToWindow");
				throw std::runtime_error("NaN encountered in addPhaseNoiseToWindow");
			}
		}
	}
}

namespace serial
{
	void exportReceiverXml(std::span<const std::unique_ptr<Response>> responses, const std::string& filename)
	{
		// Create a new XML document
		const XmlDocument doc;

		// Create the root element for the document
		xmlNodePtr root_node = xmlNewNode(nullptr, reinterpret_cast<const xmlChar*>("receiver"));
		XmlElement root(root_node);

		// Set the root element for the document
		doc.setRootElement(root);

		// Iterate over the responses and call renderXml on each, passing the root element
		for (const auto& response : responses)
		{
			response->renderXml(root); // Assuming renderXml modifies the XML tree
		}

		// Save the document to file
		if (const fs::path file_path = fs::path(filename).replace_extension(".fersxml"); !doc.
			saveFile(file_path.string()))
		{
			LOG(logging::Level::FATAL, "Failed to save XML file: {}", file_path.string());
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

				if (!*of)
				{
					LOG(logging::Level::FATAL, "Could not open file {} for writing", file_path.string());
					throw std::runtime_error("Could not open file " + file_path.string() + " for writing");
				}

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

		// Open HDF5 file for writing
		std::optional<HighFive::File> out_bin = openHdf5File(recvName);

		// Create a threaded render object to manage the rendering process
		const ThreadedResponseRenderer thr_renderer(responses, recv, params::renderThreads());

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
			auto pnoise = generatePhaseNoise(recv, size, rate, carrier, pn_enabled);

			// Get the window start time, including clock drift effects
			RealType start = recv->getWindowStart(i) + pnoise[0] / (2 * std::numbers::pi * carrier);

			// Calculate the fractional delay using structured bindings
			const auto [round_start, frac_delay] = [&start, rate]
			{
				RealType rounded_start = std::round(start * rate) / rate;
				RealType fractional_delay = start * rate - std::round(start * rate);
				return std::tuple{rounded_start, fractional_delay};
			}();
			start = round_start;

			// Allocate memory for the window using std::vector
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

				// Perform downsampling
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

			// Export the binary format if enabled, using HighFive to write to the HDF5 file
			if (params::exportBinary() && out_bin)
			{
				try
				{
					// Use HighFive to add the data chunks to the HDF5 file
					serial::addChunkToFile(*out_bin, window, size, start, params::rate(), fullscale, i);
				}
				catch (const std::exception& e)
				{
					LOG(logging::Level::FATAL, "Error writing chunk to HDF5 file: {}", e.what());
					throw std::runtime_error("Error writing chunk to HDF5 file: " + std::string(e.what()));
				}
			}
		}
	}
}
