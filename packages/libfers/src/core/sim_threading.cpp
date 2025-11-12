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
 * This file contains the logic for the unified, event-driven simulation loop.
 * It manages the processing of discrete time events, calculates physics for
 * continuous-wave systems between events, and dispatches tasks to worker threads.
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
#include "sim_events.h"
#include "thread_pool.h"
#include "processing/finalizer.h"
#include "simulation/channel_model.h"

using radar::Transmitter;
using radar::Receiver;
using radar::Target;
using radar::OperationMode;
using logging::Level;

namespace core
{
	void runEventDrivenSim(World* world, pool::ThreadPool& pool,
	                       const std::function<void(const std::string&, int, int)>& progress_callback)
	{
		auto& event_queue = world->getEventQueue();
		auto& sim_state = world->getSimulationState();
		const RealType end_time = params::endTime();
		const RealType dt_sim = 1.0 / (params::rate() * params::oversampleRatio());

		if (progress_callback) { progress_callback("Initializing event-driven simulation...", 0, 100); }

		// Phase 3: Start dedicated finalizer threads for each pulsed receiver
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

		// 6. Start Main Simulation Loop
		while (!event_queue.empty() && sim_state.t_current <= end_time)
		{
			// 7. Advance Clock and Process Events
			const Event current_event = event_queue.top();
			event_queue.pop();

			const RealType t_event = current_event.timestamp;

			// 8. Calculate Raw Samples for Active CW Receivers (Time-Stepped Inner Loop)
			if (t_event > sim_state.t_current)
			{
				const RealType t_next_event = t_event;
				for (RealType t_step = sim_state.t_current; t_step < t_next_event; t_step += dt_sim)
				{
					for (const auto& receiver_ptr : world->getReceivers())
					{
						if (receiver_ptr->getMode() == OperationMode::CW_MODE && receiver_ptr->isActive())
						{
							ComplexType total_sample{0.0, 0.0};
							// 8.3.1 Query active CW sources
							for (const auto& cw_source : sim_state.active_cw_transmitters)
							{
								// 8.3.2 Calculate direct and reflected paths
								total_sample += simulation::calculateDirectPathContribution(
									cw_source, receiver_ptr.get(), t_step);
								for (const auto& target_ptr : world->getTargets())
								{
									total_sample += simulation::calculateReflectedPathContribution(
										cw_source, receiver_ptr.get(), target_ptr.get(), t_step);
								}
							}
							// 8.3.3 Store sample in buffer
							const auto sample_index = static_cast<size_t>((t_step - params::startTime()) / dt_sim);
							receiver_ptr->setCwSample(sample_index, total_sample);
						}
					}
				}
			}

			sim_state.t_current = t_event;

			// 7. Process event by type
			switch (current_event.type)
			{
			case EventType::TxPulsedStart:
			{
				auto* tx = static_cast<Transmitter*>(current_event.source_object);
				// 7.1.1 Iterate through every receiver
				for (const auto& rx_ptr : world->getReceivers())
				{
					// 7.1.2 Calculate unique Response objects
					// Direct path
					if (!rx_ptr->checkFlag(Receiver::RecvFlag::FLAG_NODIRECT))
					{
						if (auto response = simulation::calculateResponse(tx, rx_ptr.get(), tx->getSignal(),
						                                                  t_event))
						{
							if (rx_ptr->getMode() == OperationMode::PULSED_MODE)
							{
								rx_ptr->addResponseToInbox(std::move(response));
							}
							else { rx_ptr->addInterferenceToLog(std::move(response)); }
						}
					}
					// Reflected paths
					for (const auto& target_ptr : world->getTargets())
					{
						if (auto response = simulation::calculateResponse(tx, rx_ptr.get(), tx->getSignal(),
						                                                  t_event, target_ptr.get()))
						{
							if (rx_ptr->getMode() == OperationMode::PULSED_MODE)
							{
								rx_ptr->addResponseToInbox(std::move(response));
							}
							else { rx_ptr->addInterferenceToLog(std::move(response)); }
						}
					}
				}
				// 7.1.4 Schedule next TxPulsedStart event
				const RealType next_pulse_time = t_event + 1.0 / tx->getPrf();
				if (next_pulse_time <= end_time)
				{
					event_queue.push({next_pulse_time, EventType::TxPulsedStart, tx});
				}
				break;
			}
			case EventType::RxPulsedWindowStart:
			{
				auto* rx = static_cast<Receiver*>(current_event.source_object);
				// 7.2.1 Mark receiver as active
				rx->setActive(true);
				// 7.2.2 Schedule RxPulsedWindowEnd event
				event_queue.push({t_event + rx->getWindowLength(), EventType::RxPulsedWindowEnd, rx});
				break;
			}
			case EventType::RxPulsedWindowEnd:
			{
				auto* rx = static_cast<Receiver*>(current_event.source_object);
				// 7.3.1 Mark receiver as inactive
				rx->setActive(false);
				// 7.3.2 Create RenderingJob
				RenderingJob job{
					.ideal_start_time = t_event - rx->getWindowLength(),
					.duration = rx->getWindowLength(),
					.responses = rx->drainInbox(),
					.active_cw_sources = sim_state.active_cw_transmitters
				};
				// 7.3.3 Move RenderingJob to finalizer queue
				rx->enqueueFinalizerJob(std::move(job));
				// 7.3.4 Schedule next RxPulsedWindowStart event
				const RealType next_window_start = t_event - rx->getWindowLength() + 1.0 / rx->getWindowPrf();
				if (next_window_start <= end_time)
				{
					event_queue.push({next_window_start, EventType::RxPulsedWindowStart, rx});
				}
				break;
			}
			case EventType::TxCwStart:
			{
				// 7.4.1 Add to global list of active CW sources
				auto* tx = static_cast<Transmitter*>(current_event.source_object);
				sim_state.active_cw_transmitters.push_back(tx);
				break;
			}
			case EventType::TxCwEnd:
			{
				// 7.5.1 Remove from global list
				auto* tx = static_cast<Transmitter*>(current_event.source_object);
				std::erase(sim_state.active_cw_transmitters, tx);
				break;
			}
			case EventType::RxCwStart:
			{
				// 7.6.1 Mark receiver as active
				auto* rx = static_cast<Receiver*>(current_event.source_object);
				rx->setActive(true);
				break;
			}
			case EventType::RxCwEnd:
			{
				// 7.7.1 Mark receiver as inactive
				auto* rx = static_cast<Receiver*>(current_event.source_object);
				rx->setActive(false);
				// 7.7.2 Enqueue finalization task
				pool.enqueue(processing::finalizeCwReceiver, rx, &pool);
				break;
			}
			}

			if (progress_callback)
			{
				const int progress = static_cast<int>(sim_state.t_current / end_time * 100.0);
				progress_callback(std::format("Simulating... {:.2f}s / {:.2f}s", sim_state.t_current, end_time),
				                  progress, 100);
			}
		}

		// Phase 4: Shutdown
		LOG(Level::INFO, "Main simulation loop finished. Waiting for finalization tasks...");

		// Signal pulsed finalizer threads to shut down
		for (const auto& receiver_ptr : world->getReceivers())
		{
			if (receiver_ptr->getMode() == OperationMode::PULSED_MODE)
			{
				// Enqueue a "poison pill" job to terminate the finalizer loop
				RenderingJob shutdown_job{.duration = -1.0};
				receiver_ptr->enqueueFinalizerJob(std::move(shutdown_job));
			}
		}

		// Wait for CW finalization tasks in the main pool to complete
		pool.wait();

		// The std::jthread finalizer threads will be automatically joined here at the end of scope.
		LOG(Level::INFO, "All finalization tasks complete.");

		if (progress_callback) { progress_callback("Simulation complete", 100, 100); }
		LOG(Level::INFO, "Event-driven simulation loop finished.");
	}
}
