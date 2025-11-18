// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
 * @file signal_processor.cpp
 * @brief Implementation for receiver-side signal processing and rendering.
 *
 * This file contains the implementation of functions that perform digital signal
 * processing on received radar signals, as declared in signal_processor.h.
 */

#include "signal_processor.h"

#include <algorithm>
#include <cmath>
#include <future>
#include <queue>
#include <tuple>
#include <vector>

#include <libfers/logging.h>
#include <libfers/parameters.h>
#include <libfers/response.h>
#include "core/thread_pool.h"
#include "noise/noise_generators.h"

namespace
{
	/// Minimum number of responses required to justify parallel processing for rendering.
	constexpr unsigned MIN_RESPONSES_FOR_PARALLEL_RENDERING = 8;

	/**
	 * @brief Simulate an ADC quantization process on a window of complex samples.
	 * @param data A span of ComplexType objects representing the window to quantize.
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
	 * @brief Process a single response and add its rendered data to a local window buffer.
	 * @param resp A pointer to a Response object to process.
	 * @param localWindow A reference to a vector of ComplexType representing the local window.
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
	 * @brief Process a list of responses sequentially in a single thread.
	 * @param workList A queue of Response pointers to process.
	 * @param window The main window buffer to which results are added.
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
	 * @brief Process a list of responses in parallel using a thread pool.
	 * @param workList A queue of Response pointers to process.
	 * @param window The main window buffer to which results are added.
	 * @param rate The sample rate in Hz.
	 * @param start The start time of the window in seconds.
	 * @param fracDelay The fractional delay of the window in seconds.
	 * @param localWindowSize The size of the local window in samples.
	 * @param pool A reference to the ThreadPool.
	 * @param numThreads The number of threads to use.
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
}

namespace processing
{
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

	void applyThermalNoise(std::span<ComplexType> window, const RealType noiseTemperature, std::mt19937& rngEngine)
	{
		if (noiseTemperature == 0) { return; }

		const RealType b = params::rate() / (2.0 * params::oversampleRatio());
		const RealType total_power = params::boltzmannK() * noiseTemperature * b;

		// Split total power equally between the I and Q channels
		const RealType per_channel_power = total_power / 2.0;
		const RealType stddev = std::sqrt(per_channel_power);

		noise::WgnGenerator generator(rngEngine, stddev);
		for (auto& sample : window)
		{
			sample += ComplexType(generator.getSample(), generator.getSample());
		}
	}

	RealType quantizeAndScaleWindow(std::span<ComplexType> window)
	{
		RealType max_value = 0;
		for (const auto& sample : window)
		{
			const RealType real_abs = std::fabs(sample.real());
			const RealType imag_abs = std::fabs(sample.imag());

			max_value = std::max({max_value, real_abs, imag_abs});
		}

		if (const auto adc_bits = params::adcBits(); adc_bits > 0) { adcSimulate(window, adc_bits, max_value); }
		else if (max_value != 0)
		{
			for (auto& sample : window)
			{
				sample /= max_value;
			}
		}
		return max_value;
	}
}
