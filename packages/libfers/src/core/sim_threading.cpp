// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
 * @file sim_threading.cpp
 * @brief Implementation of the main simulation runner.
 *
 * This file contains the logic for orchestrating multi-threaded radar simulations.
 * It manages the distribution of simulation tasks (e.g., for each Tx/Rx pair or
 * for each time sample) to a thread pool and coordinates the overall progress
 * of the simulation from start to finish.
 */

#include "sim_threading.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <format>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <libfers/logging.h>
#include <libfers/parameters.h>
#include <libfers/receiver.h>
#include <libfers/response.h>
#include <libfers/target.h>
#include <libfers/transmitter.h>
#include <libfers/world.h>
#include "thread_pool.h"
#include "interpolation/interpolation_point.h"
#include "simulation/channel_model.h"

using radar::Transmitter;
using radar::Receiver;
using radar::Target;
using radar::TransmitterPulse;
using serial::Response;
using logging::Level;
using simulation::ReResults;
using simulation::RangeError;

namespace
{
	/**
	 * @brief Creates a Response object by simulating a signal's interaction over its duration.
	 *
	 * This function iterates over the duration of a transmitted pulse, calling the
	 * appropriate channel model function (`solveRe` or `solveReDirect`) at discrete
	 * time steps to generate a series of `InterpPoint`s. These points capture the
	 * time-varying properties of the received signal and are collected into a `Response` object.
	 *
	 * @param trans Pointer to the transmitter.
	 * @param recv Pointer to the receiver.
	 * @param signal Pointer to the transmitted pulse.
	 * @param targ Optional pointer to a target. If null, a direct path is simulated.
	 * @throws RangeError If the channel model reports an invalid geometry (e.g., objects too close).
	 * @throws std::runtime_error If the simulation parameters result in zero time steps.
	 */
	void simulateResponse(const Transmitter* trans, Receiver* recv, const TransmitterPulse* signal,
	                      const Target* targ = nullptr)
	{
		if (targ == nullptr && trans->getAttached() == recv) { return; }

		const auto start_time = std::chrono::duration<RealType>(signal->time);
		const auto end_time = start_time + std::chrono::duration<RealType>(signal->wave->getLength());
		const auto sample_time = std::chrono::duration<RealType>(1.0 / params::simSamplingRate());
		const int point_count = static_cast<int>(std::ceil(signal->wave->getLength() / sample_time.count()));

		if (targ && point_count == 0)
		{
			LOG(Level::FATAL, "No time points are available for execution!");
			throw std::runtime_error("No time points are available for execution!");
		}

		auto response = std::make_unique<Response>(signal->wave, trans);

		try
		{
			for (int i = 0; i <= point_count; ++i)
			{
				const auto current_time = i < point_count ? start_time + i * sample_time : end_time;

				ReResults results{};
				if (targ)
				{
					simulation::solveRe(trans, recv, targ, current_time, sample_time, signal->wave, results);
				}
				else
				{
					simulation::solveReDirect(trans, recv, current_time, sample_time, signal->wave, results);
				}

				interp::InterpPoint point{
					.power = results.power,
					.time = current_time.count() + results.delay,
					.delay = results.delay,
					.doppler_factor = results.doppler_factor,
					.phase = results.phase,
					.noise_temperature = results.noise_temperature
				};
				response->addInterpPoint(point);
			}
		}
		catch (const RangeError&)
		{
			LOG(Level::FATAL, "Receiver or Transmitter too close for accurate simulation");
			throw; // Re-throw to be caught by the runner
		}

		recv->addResponse(std::move(response));
	}

	/**
	 * @brief Simulates all interactions for a single transmitter-receiver pair over the simulation time.
	 *
	 * For a given pair, this function iterates through all transmitted pulses. For each
	 * pulse, it simulates the reflected path from every target in the world and, if
	 * enabled, the direct path from the transmitter.
	 *
	 * @param trans Pointer to the transmitter.
	 * @param recv Pointer to the receiver.
	 * @param world Pointer to the simulation environment.
	 */
	void simulatePair(const Transmitter* trans, Receiver* recv, const core::World* world)
	{
		constexpr auto flag_nodirect = Receiver::RecvFlag::FLAG_NODIRECT;

		TransmitterPulse pulse{};

		const int pulses = trans->getPulseCount();

		for (int i = 0; i < pulses; ++i)
		{
			trans->setPulse(&pulse, i);

			std::ranges::for_each(world->getTargets(), [&](const auto& target)
			{
				simulateResponse(trans, recv, &pulse, target.get());
			});

			if (!recv->checkFlag(flag_nodirect)) { simulateResponse(trans, recv, &pulse); }
		}
	}
}

namespace core
{
	void runThreadedSim(const World* world, pool::ThreadPool& pool,
	                    const std::function<void(const std::string&, int, int)>& progress_callback)
	{
		const auto& receivers = world->getReceivers();
		const auto& transmitters = world->getTransmitters();
		const unsigned total_pairs = receivers.size() * transmitters.size();
		const unsigned total_renders = receivers.size();
		const unsigned total_steps = total_pairs + total_renders;

		if (progress_callback) { progress_callback("Initializing Pulsed simulation...", 0, total_steps); }

		LOG(Level::INFO, "Running radar simulation for {} Tx/Rx pairs", total_pairs);

		std::atomic<unsigned> pairs_completed = 0;
		for (const auto& receiver : receivers)
		{
			for (const auto& transmitter : transmitters)
			{
				pool.enqueue([&]
				{
					simulatePair(transmitter.get(), receiver.get(), world);
					unsigned completed = ++pairs_completed;
					if (progress_callback)
					{
						progress_callback(std::format("Simulating Tx/Rx pairs ({}/{})", completed, total_pairs),
						                  completed, total_steps);
					}
				});
			}
		}
		pool.wait();

		for (const auto& receiver : receivers)
		{
			LOG(Level::DEBUG, "{} responses added to '{}'", receiver->getResponseCount(), receiver->getName());
		}

		LOG(Level::INFO, "Rendering responses for {} receivers", total_renders);

		std::atomic<unsigned> renders_completed = 0;
		for (const auto& receiver : receivers)
		{
			pool.enqueue([&]
			{
				receiver->render(pool);
				unsigned completed = ++renders_completed;
				if (progress_callback)
				{
					progress_callback(std::format("Rendering receivers ({}/{})", completed, total_renders),
					                  total_pairs + completed, total_steps);
				}
			});
		}
		pool.wait();

		if (progress_callback) { progress_callback("Simulation complete", total_steps, total_steps); }
	}

	void runThreadedCwSim(const World* world, pool::ThreadPool& pool,
	                      const std::function<void(const std::string&, int, int)>& progress_callback)
	{
		const auto& receivers = world->getReceivers();
		const auto& transmitters = world->getTransmitters();
		const auto& targets = world->getTargets();

		const RealType start_time = params::startTime();
		const RealType end_time = params::endTime();
		const RealType dt = 1.0 / params::rate();
		const auto num_samples = static_cast<size_t>(std::ceil((end_time - start_time) / dt));
		const unsigned total_renders = receivers.size();
		const unsigned total_steps = num_samples + total_renders;

		if (progress_callback) { progress_callback("Initializing CW simulation...", 0, total_steps); }

		LOG(Level::INFO, "Running CW simulation for {} receivers over {} samples", receivers.size(), num_samples);

		for (const auto& receiver : receivers)
		{
			receiver->prepareCwData(num_samples);
		}

		std::atomic<size_t> samples_completed = 0;
		for (size_t sample_index = 0; sample_index < num_samples; ++sample_index)
		{
			const RealType t_k = start_time + sample_index * dt;
			pool.enqueue([&, t_k, sample_index]
			{
				for (const auto& receiver : receivers)
				{
					ComplexType total_sample{0.0, 0.0};
					for (const auto& transmitter : transmitters)
					{
						if (!receiver->checkFlag(Receiver::RecvFlag::FLAG_NODIRECT))
						{
							total_sample += simulation::calculateDirectPathContribution(
								transmitter.get(), receiver.get(), t_k);
						}
						for (const auto& target : targets)
						{
							total_sample += simulation::calculateReflectedPathContribution(
								transmitter.get(), receiver.get(), target.get(), t_k);
						}
					}
					receiver->setCwSample(sample_index, total_sample);
				}

				size_t completed = ++samples_completed;
				if (progress_callback)
				{
					if (completed % (num_samples / 100 + 1) == 0 || completed == num_samples)
					{
						progress_callback(std::format("Calculating samples ({}/{})", completed, num_samples),
						                  completed, total_steps);
					}
				}
			});
		}
		pool.wait();

		LOG(Level::INFO, "Rendering CW data for {} receivers", total_renders);

		std::atomic<unsigned> renders_completed = 0;
		for (const auto& receiver : receivers)
		{
			pool.enqueue([&]
			{
				receiver->render(pool);
				unsigned completed = ++renders_completed;
				if (progress_callback)
				{
					progress_callback(std::format("Rendering CW data ({}/{})", completed, total_renders),
					                  num_samples + completed, total_steps);
				}
			});
		}
		pool.wait();

		if (progress_callback) { progress_callback("CW simulation complete", total_steps, total_steps); }
	}
}
