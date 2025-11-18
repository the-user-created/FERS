// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
 * @file finalizer.cpp
 * @brief Implements the asynchronous receiver data processing and output pipelines.
 *
 * This file contains the core logic for finalizing received radar data.
 * Finalization is performed asynchronously to the main simulation loop to avoid
 * blocking physics calculations with expensive tasks like signal rendering,
 * processing, and file I/O.
 *
 * Two distinct finalization pipelines are implemented:
 * 1.  `runPulsedFinalizer`: A long-running function executed in a dedicated
 *     thread for each pulsed-mode receiver. It processes data in chunks
 *     (`RenderingJob`) as they become available from the simulation loop.
 * 2.  `finalizeCwReceiver`: A one-shot task submitted to the main thread pool
 *     for each continuous-wave receiver at the end of its operation. It processes
 *     the entire collected data buffer at once.
 *
 * Both pipelines apply effects like thermal noise, phase noise (jitter),
 * interference, downsampling, and ADC quantization before writing the final
 * I/Q data to an HDF5 file.
 */

#include "finalizer.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <ranges>
#include <tuple>
#include <highfive/highfive.hpp>

#include <libfers/logging.h>
#include <libfers/parameters.h>
#include <libfers/receiver.h>
#include <libfers/target.h>
#include <libfers/transmitter.h>
#include "core/rendering_job.h"
#include "processing/signal_processor.h"
#include "serial/hdf5_handler.h"
#include "signal/dsp_filters.h"
#include "simulation/channel_model.h"
#include "timing/timing.h"

namespace
{
	/**
	 * @brief Applies phase noise to a window of complex I/Q samples.
	 * @param noise A span of phase noise samples in radians.
	 * @param window The window of complex I/Q samples to modify.
	 */
	void addPhaseNoiseToWindow(std::span<const RealType> noise, std::span<ComplexType> window)
	{
		for (auto [n, w] : std::views::zip(noise, window))
		{
			w *= std::polar(1.0, n);
		}
	}
}

namespace processing
{
	void runPulsedFinalizer(radar::Receiver* receiver, pool::ThreadPool* pool,
	                        const std::vector<std::unique_ptr<radar::Target>>* targets)
	{
		// Each finalizer thread gets a private, stateful clone of the timing model
		// to ensure thread safety and independent state progression.
		const auto timing_model = receiver->getTiming()->clone();
		if (!timing_model)
		{
			LOG(logging::Level::FATAL, "Failed to clone timing model for receiver '{}'", receiver->getName());
			return;
		}

		const auto hdf5_filename = std::format("{}_results.h5", receiver->getName());
		HighFive::File h5_file(hdf5_filename, HighFive::File::Truncate);
		unsigned chunk_index = 0;
		LOG(logging::Level::INFO, "Finalizer thread started for receiver '{}'. Outputting to '{}'.",
		    receiver->getName(), hdf5_filename);

		// Main processing loop for this receiver's dedicated thread.
		while (true)
		{
			core::RenderingJob job;
			if (!receiver->waitAndDequeueFinalizerJob(job))
			{
				break; // Shutdown signal ("poison pill" job) received.
			}

			// Process the RenderingJob for one receive window.
			const RealType rate = params::rate() * params::oversampleRatio();
			const RealType dt = 1.0 / rate;

			const auto window_samples = static_cast<unsigned>(std::ceil(job.duration * rate));
			std::vector pnoise(window_samples, 0.0);

			RealType actual_start = job.ideal_start_time;

			if (timing_model->isEnabled())
			{
				// Advance the private clock model to the start of the current window.
				if (timing_model->getSyncOnPulse())
				{
					// For sync-on-pulse models, reset phase and skip to the window start.
					timing_model->reset();
					timing_model->skipSamples(static_cast<int>(std::floor(rate * receiver->getWindowSkip())));
				}
				else // TODO: should we use (else if chunk_index > 0) here to avoid skipping on the first window?
				{
					// For free-running models, skip the "dead time" between windows.
					const RealType inter_pulse_skip_duration = 1.0 / receiver->getWindowPrf() - receiver->
						getWindowLength();
					const auto samples_to_skip = static_cast<long>(std::floor(rate * inter_pulse_skip_duration));
					timing_model->skipSamples(samples_to_skip);
				}

				// Generate phase noise for the entire window.
				std::ranges::generate(pnoise, [&] { return timing_model->getNextSample(); });

				// The first phase noise sample determines the time jitter for this window.
				const RealType carrier = timing_model->getFrequency();
				actual_start += pnoise[0] / (2.0 * PI * carrier);
			}

			// Decompose the jittered start time into a sample-aligned start and a
			// fractional delay, which is passed to the rendering engine.
			RealType frac_delay;
			std::tie(actual_start, frac_delay) = [&actual_start, rate]
			{
				RealType rounded_start = std::round(actual_start * rate) / rate;
				RealType fractional_delay = actual_start * rate - std::round(actual_start * rate);
				return std::tuple{rounded_start, fractional_delay};
			}();

			// --- Signal Rendering and Processing Pipeline ---
			std::vector<ComplexType> window_buffer(window_samples);

			// 1. Apply thermal noise.
			applyThermalNoise(window_buffer, receiver->getNoiseTemperature(receiver->getRotation(actual_start)),
			                  receiver->getRngEngine());

			// 2. Add interference from active continuous-wave sources.
			RealType t_sample = actual_start;
			for (auto& window_sample : window_buffer)
			{
				ComplexType cw_interference_sample{0.0, 0.0};
				for (const auto* cw_source : job.active_cw_sources)
				{
					// TODO: use nodirect?
					if (!receiver->checkFlag(radar::Receiver::RecvFlag::FLAG_NODIRECT))
					{
						cw_interference_sample += simulation::calculateDirectPathContribution(
							cw_source, receiver, t_sample);
					}
					for (const auto& target_ptr : *targets)
					{
						cw_interference_sample += simulation::calculateReflectedPathContribution(
							cw_source, receiver, target_ptr.get(), t_sample);
					}
				}
				window_sample += cw_interference_sample;
				t_sample += dt;
			}

			// 3. Render the primary pulsed responses.
			renderWindow(window_buffer, job.duration, actual_start, frac_delay, job.responses, *pool);

			// 4. Apply phase noise (jitter).
			if (timing_model->isEnabled()) { addPhaseNoiseToWindow(pnoise, window_buffer); }

			// --- Finalization and Output ---
			// 5. Downsample if oversampling was used.
			if (params::oversampleRatio() > 1) { window_buffer = std::move(fers_signal::downsample(window_buffer)); }

			// 6. Quantize and scale to simulate ADC effects.
			const RealType fullscale = quantizeAndScaleWindow(window_buffer);

			// 7. Write the processed chunk to the HDF5 file.
			serial::addChunkToFile(h5_file, window_buffer, actual_start, fullscale, chunk_index++);
		}

		LOG(logging::Level::INFO, "Finalizer thread for receiver '{}' finished.", receiver->getName());
	}

	void finalizeCwReceiver(radar::Receiver* receiver, pool::ThreadPool* pool)
	{
		LOG(logging::Level::INFO, "Finalization task started for CW receiver '{}'.", receiver->getName());

		// Process the entire collected I/Q buffer for the CW receiver.
		auto& iq_buffer = receiver->getMutableCwData();
		const auto& interference_log = receiver->getPulsedInterferenceLog();

		if (iq_buffer.empty())
		{
			LOG(logging::Level::INFO, "No CW data to finalize for receiver '{}'.", receiver->getName());
			return;
		}

		// --- Signal Rendering and Processing Pipeline ---
		// 1. Render pulsed interference signals into the main I/Q buffer.
		for (const auto& response : interference_log)
		{
			unsigned psize;
			RealType prate;
			const auto rendered_pulse = response->renderBinary(prate, psize, 0.0);

			const RealType dt_sim = 1.0 / prate;
			const auto start_index = static_cast<size_t>((response->startTime() - params::startTime()) / dt_sim);

			for (size_t i = 0; i < psize; ++i)
			{
				if (start_index + i < iq_buffer.size()) { iq_buffer[start_index + i] += rendered_pulse[i]; }
			}
		}

		// Clone the timing model to ensure thread safety.
		const auto timing_model = receiver->getTiming()->clone();
		if (!timing_model)
		{
			LOG(logging::Level::FATAL, "Failed to clone timing model for CW receiver '{}'", receiver->getName());
			return;
		}

		// 2. Apply thermal noise.
		applyThermalNoise(iq_buffer, receiver->getNoiseTemperature(), receiver->getRngEngine());

		// 3. Generate and apply a single continuous phase noise sequence.
		std::vector pnoise(iq_buffer.size(), 0.0);
		if (timing_model->isEnabled())
		{
			std::ranges::generate(pnoise, [&] { return timing_model->getNextSample(); });
			addPhaseNoiseToWindow(pnoise, iq_buffer);
		}

		// --- Finalization and Output ---
		// 4. Downsample if oversampling was used.
		if (params::oversampleRatio() > 1) { iq_buffer = std::move(fers_signal::downsample(iq_buffer)); }

		// 5. Apply ADC quantization and scaling.
		const RealType fullscale = quantizeAndScaleWindow(iq_buffer);

		// 6. Write the entire processed buffer to an HDF5 file.
		const auto hdf5_filename = std::format("{}_results.h5", receiver->getName());
		try
		{
			HighFive::File file(hdf5_filename, HighFive::File::Truncate);

			std::vector<RealType> i_data(iq_buffer.size());
			std::vector<RealType> q_data(iq_buffer.size());
			std::ranges::transform(iq_buffer, i_data.begin(), [](const auto& c) { return c.real(); });
			std::ranges::transform(iq_buffer, q_data.begin(), [](const auto& c) { return c.imag(); });

			HighFive::DataSet i_dataset = file.createDataSet<RealType>("I_data", HighFive::DataSpace::From(i_data));
			i_dataset.write(i_data);
			HighFive::DataSet q_dataset = file.createDataSet<RealType>("Q_data", HighFive::DataSpace::From(q_data));
			q_dataset.write(q_data);

			file.createAttribute("sampling_rate", params::rate());
			file.createAttribute("start_time", params::startTime());
			file.createAttribute("fullscale", fullscale);
			file.createAttribute("reference_carrier_frequency", timing_model->getFrequency());

			LOG(logging::Level::INFO, "Successfully exported CW data for receiver '{}' to '{}'", receiver->getName(),
			    hdf5_filename);
		}
		catch (const HighFive::Exception& err)
		{
			LOG(logging::Level::FATAL, "Error writing CW data to HDF5 file '{}': {}", hdf5_filename, err.what());
		}
	}
}
