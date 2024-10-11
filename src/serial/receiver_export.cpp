/**
 * @file receiver_export.cpp
 * @brief Export receiver data to various formats.
 *
 * @authors David Young, Marc Brooker
 * @date 2024-09-13
 */

#include "receiver_export.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <future>
#include <map>
#include <optional>
#include <queue>
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
#include "core/parameters.h"
#include "core/thread_pool.h"
#include "highfive/H5Exception.hpp"
#include "libxml/xmlstring.h"
#include "noise/noise_generators.h"
#include "noise/noise_utils.h"
#include "radar/receiver.h"
#include "serial/response.h"
#include "signal/dsp_filters.h"
#include "timing/timing.h"

namespace fs = std::filesystem;

namespace
{
	/**
	 * @brief Open an HDF5 file for writing.
	 *
	 * Opens an HDF5 file for writing, overwriting the file if it already exists.
	 *
	 * @param recvName The name of the receiver, which will be used as the filename for the HDF5 export.
	 * @return An optional HighFive::File object representing the opened file, or std::nullopt if binary export is disabled.
	 * @throws std::runtime_error If the file cannot be opened.
	 */
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

	/**
	 * @brief Add noise to a window of complex samples.
	 *
	 * Adds white Gaussian noise to a window of complex samples based on the specified temperature.
	 *
	 * @param data A span of ComplexType objects representing the window of complex samples to add noise to.
	 * @param temperature The noise temperature in Kelvin. If the temperature is 0, no noise is added.
	 */
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

	/**
	 * @brief Simulate an ADC quantization process on a window of complex samples.
	 *
	 * Simulates an ADC quantization process on a window of complex samples, rounding each sample to the nearest
	 * quantization level based on the number of bits and full-scale value.
	 *
	 * @param data A span of ComplexType objects representing the window of complex samples to quantize.
	 * @param bits The number of bits used for quantization.
	 * @param fullscale The full-scale value used for quantization.
	 */
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

	/**
	 * @brief Quantize a window of complex samples to the nearest quantization level.
	 *
	 * Quantizes a window of complex samples to the nearest quantization level based on the maximum absolute value
	 * across the real and imaginary parts of the samples. The samples are normalized to the full-scale value
	 * before quantization.
	 *
	 * @param data A span of ComplexType objects representing the window of complex samples to quantize.
	 * @return The full-scale value used for quantization.
	 */
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

	/**
	 * @brief Generate phase noise samples for a window.
	 *
	 * Generates phase noise samples for a window based on the receiver timing model, window size, and sample rate.
	 * The phase noise samples are used to modulate the phase of the window samples.
	 *
	 * @param recv A pointer to the radar::Receiver object containing receiver data.
	 * @param wSize The size of the window in samples.
	 * @param rate The sample rate in Hz.
	 * @param carrier The carrier frequency of the phase noise in Hz.
	 * @param enabled A boolean flag indicating whether phase noise is enabled.
	 * @return A vector of RealType objects representing the phase noise samples.
	 */
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

	/**
	 * @brief Add phase noise to a window of complex samples.
	 *
	 * Adds phase noise to a window of complex samples by modulating the phase of each sample using the phase noise
	 * samples. The phase noise samples are generated based on the receiver timing model and window size.
	 *
	 * @param noise A span of RealType objects representing the phase noise samples.
	 * @param window A span of ComplexType objects representing the window of complex samples to add phase noise to.
	 */
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

	/**
	 * @brief Render a window of complex samples using multiple threads.
	 *
	 * Renders a window of complex samples using multiple threads, processing each response in parallel and merging
	 * the results into the shared window. The window is rendered based on the specified length, start time, fractional
	 * delay, and list of responses.
	 *
	 * @param window A reference to a vector of ComplexType objects representing the window of complex samples to render.
	 * @param length The length of the window in seconds.
	 * @param start The start time of the window in seconds.
	 * @param fracDelay The fractional delay of the window in seconds.
	 * @param responses A span of unique pointers to Response objects representing the responses to render.
	 * @param pool A reference to the ThreadPool object used for multithreading.
	 */
	void renderWindow(std::vector<ComplexType>& window, const RealType length, const RealType start,
	                  const RealType fracDelay, const std::span<const std::unique_ptr<serial::Response>> responses,
	                  pool::ThreadPool& pool)
	{
		const RealType end = start + length;
		std::queue<serial::Response*> work_list;

		// Populate the work list with relevant responses
		for (const auto& response : responses)
		{
			if (const RealType resp_end = response->endTime(); response->startTime() <= end && resp_end >= start)
			{
				work_list.push(response.get());
			}
		}

		unsigned num_responses = work_list.size();
		unsigned available_threads = pool.getAvailableThreads();
		const RealType rate = params::rate() * params::oversampleRatio();
		auto local_window_size = static_cast<unsigned>(std::ceil(length * rate));

		// Sequential processing if responses are small or threads is limited
		// TODO: Fine tune the response threshold
		if (num_responses < 10 || available_threads <= 1)
		{
			std::vector local_window(local_window_size, ComplexType{});

			while (!work_list.empty())
			{
				const auto* resp = work_list.front();
				work_list.pop();

				unsigned psize;
				RealType prate;
				auto array = resp->renderBinary(prate, psize, fracDelay);
				int start_sample = static_cast<int>(std::round(rate * (resp->startTime() - start)));
				const unsigned roffset = start_sample < 0 ? -start_sample : 0;
				if (start_sample < 0) { start_sample = 0; }

				for (unsigned i = roffset; i < psize && i + start_sample < local_window_size; ++i)
				{
					local_window[i + start_sample] += array[i];
				}
			}

			// Merge the local window into the shared window
			for (unsigned i = 0; i < local_window_size; ++i) { window[i] += local_window[i]; }
		}
		else
		{
			std::mutex window_mutex, work_list_mutex;
			auto worker = [&]
			{
				std::vector local_window(local_window_size, ComplexType{});

				while (true)
				{
					const serial::Response* resp = nullptr;
					{
						std::scoped_lock lock(work_list_mutex);
						if (work_list.empty()) { break; }
						resp = work_list.front();
						work_list.pop();
					}

					unsigned psize;
					RealType prate;
					auto array = resp->renderBinary(prate, psize, fracDelay);
					int start_sample = static_cast<int>(std::round(rate * (resp->startTime() - start)));
					const unsigned roffset = start_sample < 0 ? -start_sample : 0;
					if (start_sample < 0) { start_sample = 0; }

					for (unsigned i = roffset; i < psize && i + start_sample < local_window_size; ++i)
					{
						local_window[i + start_sample] += array[i];
					}
				}

				// Merge the local window into the shared window
				std::scoped_lock lock(window_mutex);
				for (unsigned i = 0; i < local_window_size; ++i) { window[i] += local_window[i]; }
			};

			// Multithreading with thread pool
			const unsigned num_threads = std::min(available_threads, num_responses);
			LOG(logging::Level::TRACE, "Using {} threads for rendering: {} available, {} responses", num_threads,
			    available_threads, num_responses);

			std::vector<std::future<void>> futures;
			for (unsigned i = 0; i < num_threads; ++i) { futures.push_back(pool.enqueue(worker)); }

			// Wait for all threads to finish
			for (auto& future : futures) { future.get(); }
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
	                          const std::string& recvName, pool::ThreadPool& pool)
	{
		// Bail if there are no responses to export
		if (responses.empty()) { return; }

		// Open HDF5 file for writing
		std::optional<HighFive::File> out_bin = openHdf5File(recvName);

		// Retrieve the window count from the receiver
		const unsigned window_count = recv->getWindowCount();

		const RealType length = recv->getWindowLength();
		const RealType rate = params::rate() * params::oversampleRatio();
		const auto size = static_cast<unsigned>(std::ceil(length * rate));

		RealType carrier;
		bool pn_enabled;

		// Loop through each window
		for (unsigned i = 0; i < window_count; ++i)
		{
			// Generate phase noise samples for the window
			auto pnoise = generatePhaseNoise(recv, size, rate, carrier, pn_enabled);

			// Get the window start time, including clock drift effects
			RealType start = recv->getWindowStart(i) + pnoise[0] / (2 * PI * carrier);

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

			// Render the window using multiple threads
			renderWindow(window, length, start, frac_delay, responses, pool);

			// Downsample the window if the oversample ratio is greater than 1
			if (params::oversampleRatio() > 1) { window = std::move(signal::downsample(window)); }

			// Add phase noise to the window if enabled
			if (pn_enabled) { addPhaseNoiseToWindow(pnoise, window); }

			// Normalize and quantize the window
			const RealType fullscale = quantizeWindow(window);

			// Export the binary format using HighFive to write to the HDF5 file
			if (out_bin)
			{
				try
				{
					// Use HighFive to add the data chunks to the HDF5 file
					serial::addChunkToFile(*out_bin, window, start, fullscale, i);
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
