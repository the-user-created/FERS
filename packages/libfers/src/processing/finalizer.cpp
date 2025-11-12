// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
 * @file finalizer.cpp
 * @brief Implements the asynchronous receiver finalization pipelines.
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
	 * @brief Adds phase noise to a window of complex samples.
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
		// 1. Setup: Clone timing model, open HDF5 file
		auto timing_model = receiver->getTiming()->clone();
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

		// 2. Main processing loop
		while (true)
		{
			core::RenderingJob job;
			if (!receiver->waitAndDequeueFinalizerJob(job))
			{
				break; // Shutdown signal received
			}

			// 11. Process RenderingJob Sequentially
			const RealType rate = params::rate() * params::oversampleRatio();
			const RealType dt = 1.0 / rate;

			const auto window_samples = static_cast<unsigned>(std::ceil(job.duration * rate));
			std::vector pnoise(window_samples, 0.0);

			RealType actual_start = job.ideal_start_time;

			if (timing_model->isEnabled())
			{
				// Advance the private, stateful clock model to the start of the current window.
				if (timing_model->getSyncOnPulse())
				{
					// For sync-on-pulse models, reset the phase and skip to the window start delay.
					timing_model->reset();
					timing_model->skipSamples(static_cast<int>(std::floor(rate * receiver->getWindowSkip())));
				}
				else
				{
					// For free-running models, skip the "dead time" between the previous window and this one.
					const RealType inter_pulse_skip_duration = 1.0 / receiver->getWindowPrf() - receiver->
						getWindowLength();
					const auto samples_to_skip = static_cast<long>(std::floor(rate * inter_pulse_skip_duration));
					timing_model->skipSamples(samples_to_skip);
				}

				// Generate phase noise for the window
				std::ranges::generate(pnoise, [&] { return timing_model->getNextSample(); });

				// Use first noise sample to calculate jittered start time and fractional delay
				const RealType carrier = timing_model->getFrequency();
				actual_start += pnoise[0] / (2.0 * PI * carrier);
			}

			RealType frac_delay;
			std::tie(actual_start, frac_delay) = [&actual_start, rate]
			{
				RealType rounded_start = std::round(actual_start * rate) / rate;
				RealType fractional_delay = actual_start * rate - std::round(actual_start * rate);
				return std::tuple{rounded_start, fractional_delay};
			}();

			// 12. Execute Raw Signal Rendering
			std::vector<ComplexType> window_buffer(window_samples);

			// Add thermal noise
			applyThermalNoise(window_buffer, receiver->getNoiseTemperature(receiver->getRotation(actual_start)),
			                  receiver->getRngEngine());

			// Add CW interference
			for (size_t i = 0; i < window_buffer.size(); ++i)
			{
				const RealType t_sample = actual_start + i * dt;
				ComplexType cw_interference_sample{0.0, 0.0};
				for (const auto* cw_source : job.active_cw_sources)
				{
					cw_interference_sample += simulation::calculateDirectPathContribution(cw_source, receiver,
					                                                                      t_sample);
					for (const auto& target_ptr : *targets)
					{
						cw_interference_sample += simulation::calculateReflectedPathContribution(
							cw_source, receiver, target_ptr.get(), t_sample);
					}
				}
				window_buffer[i] += cw_interference_sample;
			}

			// Render pulsed responses
			renderWindow(window_buffer, job.duration, actual_start, frac_delay, job.responses, *pool);

			// Apply phase noise
			if (timing_model->isEnabled()) { addPhaseNoiseToWindow(pnoise, window_buffer); }

			// 13. Apply Final Effects and Write to File
			// Downsample if oversampling was used
			if (params::oversampleRatio() > 1) { window_buffer = std::move(fers_signal::downsample(window_buffer)); }

			// Quantize and scale
			const RealType fullscale = quantizeAndScaleWindow(window_buffer);

			// Write chunk to HDF5 file
			serial::addChunkToFile(h5_file, window_buffer, actual_start, fullscale, chunk_index++);
		}

		LOG(logging::Level::INFO, "Finalizer thread for receiver '{}' finished.", receiver->getName());
	}

	void finalizeCwReceiver(radar::Receiver* receiver, pool::ThreadPool* pool)
	{
		LOG(logging::Level::INFO, "Finalization task started for CW receiver '{}'.", receiver->getName());

		// 14. Process Full CW Buffer
		auto& iq_buffer = receiver->getMutableCwData();
		const auto& interference_log = receiver->getPulsedInterferenceLog();

		if (iq_buffer.empty())
		{
			LOG(logging::Level::INFO, "No CW data to finalize for receiver '{}'.", receiver->getName());
			return;
		}

		// Render pulsed interferences into the main buffer
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

		// Clone the timing model to ensure thread safety, especially if the model is shared.
		auto timing_model = receiver->getTiming()->clone();
		if (!timing_model)
		{
			LOG(logging::Level::FATAL, "Failed to clone timing model for CW receiver '{}'", receiver->getName());
			return;
		}

		// Apply thermal noise
		applyThermalNoise(iq_buffer, receiver->getNoiseTemperature(), receiver->getRngEngine());

		// Generate and apply a single continuous phase noise sequence from the cloned model
		std::vector pnoise(iq_buffer.size(), 0.0);
		if (timing_model->isEnabled())
		{
			std::ranges::generate(pnoise, [&] { return timing_model->getNextSample(); });
			addPhaseNoiseToWindow(pnoise, iq_buffer);
		}

		// Downsample if oversampling was used
		if (params::oversampleRatio() > 1) { iq_buffer = std::move(fers_signal::downsample(iq_buffer)); }

		// Apply ADC quantization
		quantizeAndScaleWindow(iq_buffer);

		// 15. Write CW Data to File
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
