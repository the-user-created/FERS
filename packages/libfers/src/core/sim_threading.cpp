// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
 * @file sim_threading.cpp
 * @brief Implements the core event-driven simulation engine.
 *
 * This file contains the primary simulation loop, which orchestrates the entire
 * simulation process. It operates on a unified, event-driven model capable of
 * handling both pulsed and continuous-wave (CW) radar systems concurrently.
 * The loop advances simulation time by processing events from a priority queue.
 *
 * A key feature is the time-stepped inner loop that calculates physics for
 * active CW systems between discrete events. This hybrid approach allows for
 * efficient simulation of both event-based (pulsed) and continuous (CW) phenomena.
 * To maintain performance, expensive post-processing tasks are offloaded to
 * worker threads, decoupling the main physics calculations from I/O and signal
 * rendering.
 */

#include "sim_threading.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <format>
#include <functional>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include <libfers/logging.h>
#include <libfers/parameters.h>
#include <libfers/receiver.h>
#include <libfers/target.h>
#include <libfers/transmitter.h>
#include <libfers/world.h>
#include "processing/finalizer.h"
#include "sim_events.h"
#include "simulation/channel_model.h"
#include "thread_pool.h"

using logging::Level;
using radar::OperationMode;
using radar::Receiver;
using radar::Target;
using radar::Transmitter;

namespace core
{
	void runEventDrivenSim(World* world, pool::ThreadPool& pool,
						   const std::function<void(const std::string&, int, int)>& progress_callback)
	{
		auto& event_queue = world->getEventQueue();
		auto& [t_current, active_cw_transmitters] = world->getSimulationState();
		const RealType end_time = params::endTime();
		const RealType dt_sim = 1.0 / (params::rate() * params::oversampleRatio());

		if (progress_callback)
		{
			progress_callback("Initializing event-driven simulation...", 0, 100);
		}

		// Start dedicated finalizer threads for each pulsed receiver. This creates a
		// one-thread-per-receiver pipeline for asynchronous data processing.
		std::vector<std::jthread> finalizer_threads;
		for (const auto& receiver_ptr : world->getReceivers())
		{
			if (receiver_ptr->getMode() == OperationMode::PULSED_MODE)
			{
				finalizer_threads.emplace_back(processing::runPulsedFinalizer, receiver_ptr.get(), &pool,
											   &world->getTargets());
			}
		}

		LOG(Level::INFO, "Starting unified event-driven simulation loop.");

		// Main Simulation Loop
		while (!event_queue.empty() && t_current <= end_time)
		{
			// Advance Clock to the next scheduled event
			const auto [timestamp, event_type, source_object] = event_queue.top();
			event_queue.pop();

			const RealType t_event = timestamp;

			// Before processing the event, run a time-stepped inner loop to calculate
			// physics for any active continuous-wave systems. This handles the "continuous"
			// part of the simulation between discrete events.
			if (t_event > t_current)
			{
				const RealType t_next_event = t_event;

				const auto start_index = static_cast<size_t>(std::ceil((t_current - params::startTime()) / dt_sim));
				const auto end_index = static_cast<size_t>(std::ceil((t_next_event - params::startTime()) / dt_sim));

				for (size_t sample_index = start_index; sample_index < end_index; ++sample_index)
				{
					const RealType t_step = params::startTime() + static_cast<RealType>(sample_index) * dt_sim;

					for (const auto& receiver_ptr : world->getReceivers())
					{
						if (receiver_ptr->getMode() == OperationMode::CW_MODE && receiver_ptr->isActive())
						{
							ComplexType total_sample{0.0, 0.0};
							// Query active CW sources
							for (const auto& cw_source : active_cw_transmitters)
							{
								// Calculate direct and reflected paths
								if (!receiver_ptr->checkFlag(Receiver::RecvFlag::FLAG_NODIRECT))
								{
									total_sample += simulation::calculateDirectPathContribution(
										cw_source, receiver_ptr.get(), t_step);
								}
								for (const auto& target_ptr : world->getTargets())
								{
									total_sample += simulation::calculateReflectedPathContribution(
										cw_source, receiver_ptr.get(), target_ptr.get(), t_step);
								}
							}
							// Store sample in the receiver's main buffer using the correct index
							receiver_ptr->setCwSample(sample_index, total_sample);
						}
					}
				}
			}

			t_current = t_event;

			// Process the discrete event
			switch (event_type)
			{
			case EventType::TX_PULSED_START:
				{
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): Type is guaranteed by event_type
					auto* tx = static_cast<Transmitter*>(source_object);
					// For each pulse, calculate its interaction with every receiver and target.
					for (const auto& rx_ptr : world->getReceivers())
					{
						// Calculate unique Response objects for direct and reflected paths.
						if (!rx_ptr->checkFlag(Receiver::RecvFlag::FLAG_NODIRECT))
						{
							if (auto response =
									simulation::calculateResponse(tx, rx_ptr.get(), tx->getSignal(), t_event))
							{
								if (rx_ptr->getMode() == OperationMode::PULSED_MODE)
								{
									rx_ptr->addResponseToInbox(std::move(response));
								}
								else
								{
									rx_ptr->addInterferenceToLog(std::move(response));
								}
							}
						}
						for (const auto& target_ptr : world->getTargets())
						{
							if (auto response = simulation::calculateResponse(tx, rx_ptr.get(), tx->getSignal(),
																			  t_event, target_ptr.get()))
							{
								if (rx_ptr->getMode() == OperationMode::PULSED_MODE)
								{
									rx_ptr->addResponseToInbox(std::move(response));
								}
								else
								{
									rx_ptr->addInterferenceToLog(std::move(response));
								}
							}
						}
					}
					// Schedule the next pulse transmission for this transmitter.
					if (const RealType next_pulse_time = t_event + 1.0 / tx->getPrf(); next_pulse_time <= end_time)
					{
						event_queue.push({next_pulse_time, EventType::TX_PULSED_START, tx});
					}
					break;
				}
			case EventType::RX_PULSED_WINDOW_START:
				{
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): Type is guaranteed by event_type
					auto* rx = static_cast<Receiver*>(source_object);
					rx->setActive(true);
					event_queue.push({t_event + rx->getWindowLength(), EventType::RX_PULSED_WINDOW_END, rx});
					break;
				}
			case EventType::RX_PULSED_WINDOW_END:
				{
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): Type is guaranteed by event_type
					auto* rx = static_cast<Receiver*>(source_object);
					rx->setActive(false);
					// The receive window is over. Package all received data into a RenderingJob.
					RenderingJob job{.ideal_start_time = t_event - rx->getWindowLength(),
									 .duration = rx->getWindowLength(),
									 .responses = rx->drainInbox(),
									 .active_cw_sources = active_cw_transmitters};
					// Offload the job to the dedicated finalizer thread for this receiver.
					rx->enqueueFinalizerJob(std::move(job));
					// Schedule the start of the next receive window.
					if (const RealType next_window_start = t_event - rx->getWindowLength() + 1.0 / rx->getWindowPrf();
						next_window_start <= end_time)
					{
						event_queue.push({next_window_start, EventType::RX_PULSED_WINDOW_START, rx});
					}
					break;
				}
			case EventType::TX_CW_START:
				{
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): Type is guaranteed by event_type
					auto* tx = static_cast<Transmitter*>(source_object);
					active_cw_transmitters.push_back(tx);
					break;
				}
			case EventType::TX_CW_END:
				{
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): Type is guaranteed by event_type
					auto* tx = static_cast<Transmitter*>(source_object);
					std::erase(active_cw_transmitters, tx);
					break;
				}
			case EventType::RX_CW_START:
				{
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): Type is guaranteed by event_type
					auto* rx = static_cast<Receiver*>(source_object);
					rx->setActive(true);
					break;
				}
			case EventType::RX_CW_END:
				{
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): Type is guaranteed by event_type
					auto* rx = static_cast<Receiver*>(source_object);
					rx->setActive(false);
					// The CW receiver is done. Enqueue a one-shot finalization task to the main pool.
					pool.enqueue(processing::finalizeCwReceiver, rx, &pool);
					break;
				}
			}

			if (progress_callback)
			{
				const int progress = static_cast<int>(t_current / end_time * 100.0);
				progress_callback(std::format("Simulating... {:.2f}s / {:.2f}s", t_current, end_time), progress, 100);
			}
		}

		// Shutdown Phase
		LOG(Level::INFO, "Main simulation loop finished. Waiting for finalization tasks...");

		// Signal pulsed finalizer threads to shut down by sending a "poison pill" job.
		for (const auto& receiver_ptr : world->getReceivers())
		{
			if (receiver_ptr->getMode() == OperationMode::PULSED_MODE)
			{
				RenderingJob shutdown_job{.duration = -1.0};
				receiver_ptr->enqueueFinalizerJob(std::move(shutdown_job));
			}
		}

		// Wait for any remaining CW finalization tasks in the main pool to complete.
		pool.wait();

		// The std::jthread finalizer threads are automatically joined here at the end of scope.
		LOG(Level::INFO, "All finalization tasks complete.");

		if (progress_callback)
		{
			progress_callback("Simulation complete", 100, 100);
		}
		LOG(Level::INFO, "Event-driven simulation loop finished.");
	}
}
