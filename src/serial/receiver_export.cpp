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
#include <highfive/highfive.hpp>
#include <libxml/parser.h>

#include "config.h"
#include "hdf5_handler.h"
#include "libxml_wrapper.h"
#include "core/parameters.h"
#include "core/thread_pool.h"
#include "libxml/xmlstring.h"
#include "noise/noise_generators.h"
#include "radar/receiver.h"
#include "serial/response.h"
#include "signal/dsp_filters.h"
#include "timing/timing.h"

namespace fs = std::filesystem;

namespace
{
	/// Minimum number of responses required to justify parallel processing for rendering.
	constexpr unsigned MIN_RESPONSES_FOR_PARALLEL_RENDERING = 8;

	/**
	 * @brief Open an HDF5 file for writing.
	 *
	 * @param recvName The name of the receiver, which will be used as the filename for the HDF5 export.
	 * @return An optional HighFive::File object representing the opened file, or std::nullopt if binary export is disabled.
	 * @throws std::runtime_error If the file cannot be opened.
	 */
	std::optional<HighFive::File> openHdf5File(const std::string& recvName)
	{
		const auto hdf5_filename = std::format("{}.h5", recvName);

		try
		{
			HighFive::File file(hdf5_filename, HighFive::File::Truncate);
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
	 * @param data A span of ComplexType objects representing the window of complex samples to add noise to.
	 * @param temperature The noise temperature in Kelvin. If the temperature is 0, no noise is added.
	 * @param rngEngine The random number generator engine to use for noise generation.
	 */
	void addNoiseToWindow(std::span<ComplexType> data, const RealType temperature, std::mt19937& rngEngine) noexcept
	{
		if (temperature == 0) { return; }

		// params::boltzmannK() * temperature * bandwidth
		const RealType power = params::boltzmannK() * temperature * (params::rate() * params::oversampleRatio() / 2);

		noise::WgnGenerator generator(rngEngine, std::sqrt(power) / 2.0);

		for (auto& sample : data)
		{
			const ComplexType noise_sample(generator.getSample(), generator.getSample());
			sample += noise_sample;
		}
	}

	/**
	 * @brief Simulate an ADC quantization process on a window of complex samples.
	 *
	 * @param data A span of ComplexType objects representing the window of complex samples to quantize.
	 * @param bits The number of bits used for quantization.
	 * @param fullscale The full-scale value used for quantization.
	 */
	void adcSimulate(std::span<ComplexType> data, const unsigned bits, const RealType fullscale) noexcept
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
	 * @param data A span of ComplexType objects representing the window of complex samples to quantize.
	 * @return The full-scale value used for quantization.
	 */
	RealType quantizeWindow(std::span<ComplexType> data)
	{
		RealType max_value = 0;

		for (const auto& sample : data)
		{
			const RealType real_abs = std::fabs(sample.real());
			const RealType imag_abs = std::fabs(sample.imag());

			max_value = std::max({max_value, real_abs, imag_abs});
		}

		if (const auto adc_bits = params::adcBits(); adc_bits > 0) { adcSimulate(data, adc_bits, max_value); }
		else if (max_value != 0)
		{
			for (auto& sample : data)
			{
				sample /= max_value;
			}
		}
		return max_value;
	}

	/**
	 * @brief Generate phase noise samples for a window.
	 *
	 * @param recv A pointer to the radar::Receiver object containing receiver data.
	 * @param wSize The size of the window in samples.
	 * @param rate The sample rate in Hz.
	 * @param carrier The carrier frequency of the phase noise in Hz.
	 * @param enabled A boolean flag indicating whether phase noise is enabled.
	 * @return A vector of RealType objects representing the phase noise samples.
	 */
	// NOLINTNEXTLINE(readability-non-const-parameter)
	std::vector<RealType> generatePhaseNoise(radar::Receiver* recv, const unsigned wSize, const RealType rate,
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
	 * @brief Process a response and add the response data to the window.
	 *
	 * @param resp A pointer to a Response object representing the response to process.
	 * @param localWindow A reference to a vector of ComplexType objects representing the local window of complex samples.
	 * @param rate The sample rate in Hz.
	 * @param start The start time of the window in seconds.
	 * @param fracDelay The fractional delay of the window in seconds.
	 * @param localWindowSize The size of the local window in samples.
	 */
	void processResponse(const serial::Response* resp, std::vector<ComplexType>& localWindow, const RealType rate,
	                     const RealType start, const RealType fracDelay, const unsigned localWindowSize)
	{
		unsigned psize;
		RealType prate;
		const auto array = resp->renderBinary(prate, psize, fracDelay);
		int start_sample = static_cast<int>(std::round(rate * (resp->startTime() - start)));
		const unsigned roffset = start_sample < 0 ? -start_sample : 0;
		if (start_sample < 0) { start_sample = 0; }

		for (unsigned i = roffset; i < psize && i + start_sample < localWindowSize; ++i)
		{
			localWindow[i + start_sample] += array[i];
		}
	}

	/**
	 * @brief Process responses sequentially using a single thread.
	 *
	 * @param workList A reference to a queue of Response pointers representing the list of responses to process.
	 * @param window A reference to a vector of ComplexType objects representing the window of complex samples to render.
	 * @param rate The sample rate in Hz.
	 * @param start The start time of the window in seconds.
	 * @param fracDelay The fractional delay of the window in seconds.
	 * @param localWindowSize The size of the local window in samples.
	 */
	void sequentialProcessing(std::queue<serial::Response*>& workList, std::vector<ComplexType>& window,
	                          const RealType rate, const RealType start, const RealType fracDelay,
	                          const unsigned localWindowSize)
	{
		std::vector local_window(localWindowSize, ComplexType{});

		while (!workList.empty())
		{
			const auto* resp = workList.front();
			workList.pop();
			processResponse(resp, local_window, rate, start, fracDelay, localWindowSize);
		}

		for (unsigned i = 0; i < localWindowSize; ++i) { window[i] += local_window[i]; }
	}

	/**
	 * @brief Process responses in parallel using multiple threads.
	 *
	 * @param workList A reference to a queue of Response pointers representing the list of responses to process.
	 * @param window A reference to a vector of ComplexType objects representing the window of complex samples to render.
	 * @param rate The sample rate in Hz.
	 * @param start The start time of the window in seconds.
	 * @param fracDelay The fractional delay of the window in seconds.
	 * @param localWindowSize The size of the local window in samples.
	 * @param pool A reference to the ThreadPool object used for parallel processing.
	 * @param numThreads The number of threads to use for parallel processing.
	 */
	void parallelProcessing(std::queue<serial::Response*>& workList, std::vector<ComplexType>& window,
	                        const RealType rate, const RealType start, const RealType fracDelay,
	                        const unsigned localWindowSize, pool::ThreadPool& pool, const unsigned numThreads)
	{
		std::mutex window_mutex, work_list_mutex;
		auto worker = [&]
		{
			std::vector local_window(localWindowSize, ComplexType{});

			while (true)
			{
				const serial::Response* resp = nullptr;
				{
					std::scoped_lock lock(work_list_mutex);
					if (workList.empty()) { break; }
					resp = workList.front();
					workList.pop();
				}
				processResponse(resp, local_window, rate, start, fracDelay, localWindowSize);
			}

			std::scoped_lock lock(window_mutex);
			for (unsigned i = 0; i < localWindowSize; ++i) { window[i] += local_window[i]; }
		};

		std::vector<std::future<void>> futures;
		for (unsigned i = 0; i < numThreads; ++i) { futures.push_back(pool.enqueue(worker)); }

		for (auto& future : futures) { future.get(); }
	}

	/**
	 * @brief Render a window of complex samples using multiple threads.
	 *
	 * @param window A vector of ComplexType objects representing the window of complex samples to render.
	 * @param length The length of the window in seconds.
	 * @param start The start time of the window in seconds.
	 * @param fracDelay The fractional delay of the window in seconds.
	 * @param responses A span of unique_ptr objects representing the list of responses to render.
	 * @param pool A reference to the ThreadPool object used for parallel processing.
	 */
	void renderWindow(std::vector<ComplexType>& window, const RealType length, const RealType start,
	                  const RealType fracDelay, const std::span<const std::unique_ptr<serial::Response>> responses,
	                  pool::ThreadPool& pool)
	{
		const RealType end = start + length;
		std::queue<serial::Response*> work_list;

		for (const auto& response : responses)
		{
			if (response->startTime() <= end && response->endTime() >= start) { work_list.push(response.get()); }
		}

		unsigned num_responses = work_list.size();
		unsigned available_threads = pool.getAvailableThreads();
		const RealType rate = params::rate() * params::oversampleRatio();
		const auto local_window_size = static_cast<unsigned>(std::ceil(length * rate));
		if (num_responses < MIN_RESPONSES_FOR_PARALLEL_RENDERING || available_threads <= 1)
		{
			LOG(logging::Level::TRACE, "Using sequential processing for rendering: {} threads available, {} responses",
			    available_threads, num_responses);
			sequentialProcessing(work_list, window, rate, start, fracDelay, local_window_size);
		}
		else
		{
			const unsigned num_threads = std::min(available_threads, num_responses);
			LOG(logging::Level::TRACE, "Using {} threads for rendering: {} available, {} responses", num_threads,
			    available_threads, num_responses);
			parallelProcessing(work_list, window, rate, start, fracDelay, local_window_size, pool, num_threads);
		}
	}
}

namespace serial
{
	void exportReceiverXml(const std::span<const std::unique_ptr<Response>> responses, const std::string& filename)
	{
		const XmlDocument doc;

		xmlNodePtr root_node = xmlNewNode(nullptr, reinterpret_cast<const xmlChar*>("receiver"));
		XmlElement root(root_node);

		doc.setRootElement(root);

		for (const auto& response : responses)
		{
			response->renderXml(root);
		}

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

		for (const auto& response : responses)
		{
			const std::string& transmitter_name = response->getTransmitterName();

			auto [it, inserted] = streams.try_emplace(transmitter_name, nullptr);

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

				it->second = std::move(of);
			}

			response->renderCsv(*it->second);
		}
	}

	void exportReceiverBinary(const std::span<const std::unique_ptr<Response>> responses, radar::Receiver* recv,
	                          const std::string& recvName, pool::ThreadPool& pool)
	{
		if (responses.empty()) { return; }

		std::optional<HighFive::File> out_bin = openHdf5File(recvName);

		const unsigned window_count = recv->getWindowCount();

		const RealType length = recv->getWindowLength();
		const RealType rate = params::rate() * params::oversampleRatio();
		const auto size = static_cast<unsigned>(std::ceil(length * rate));

		RealType carrier;
		bool pn_enabled;

		for (unsigned i = 0; i < window_count; ++i)
		{
			auto pnoise = generatePhaseNoise(recv, size, rate, carrier, pn_enabled);

			// Get the window start time, including clock drift effects
			RealType start = recv->getWindowStart(i) + pnoise[0] / (2 * PI * carrier);

			// Calculate the fractional delay
			const auto [round_start, frac_delay] = [&start, rate]
			{
				RealType rounded_start = std::round(start * rate) / rate;
				RealType fractional_delay = start * rate - std::round(start * rate);
				return std::tuple{rounded_start, fractional_delay};
			}();
			start = round_start;

			std::vector<ComplexType> window(size);

			addNoiseToWindow(window, recv->getNoiseTemperature(), recv->getRngEngine());

			renderWindow(window, length, start, frac_delay, responses, pool);

			if (params::oversampleRatio() > 1) { window = std::move(fers_signal::downsample(window)); }

			if (pn_enabled) { addPhaseNoiseToWindow(pnoise, window); }

			const RealType fullscale = quantizeWindow(window);

			try
			{
				addChunkToFile(*out_bin, window, start, fullscale, i);
			}
			catch (const std::exception& e)
			{
				LOG(logging::Level::FATAL, "Error writing chunk to HDF5 file: {}", e.what());
				throw std::runtime_error("Error writing chunk to HDF5 file: " + std::string(e.what()));
			}
		}
	}
}
