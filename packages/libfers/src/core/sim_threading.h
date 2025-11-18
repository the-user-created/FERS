// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
 * @file sim_threading.h
 * @brief Header file for the main simulation runner.
 *
 * This file contains the declarations for the high-level function that
 * orchestrates and manages the event-driven radar simulation.
 */

#pragma once

#include <functional>
#include <string>

namespace pool
{
	class ThreadPool;
}

namespace core
{
	class World;
}

namespace core
{
	/**
	 * @brief Runs the unified, event-driven radar simulation.
	 *
	 * This function is the core engine of the simulator. It advances time by
	 * processing events from a global priority queue. It handles both pulsed
	 * and continuous-wave (CW) physics, dispatching finalization tasks to
	 * worker threads for asynchronous processing.
	 *
	 * @param world A pointer to the simulation world containing all entities and state.
	 * @param pool A reference to the thread pool for executing tasks.
	 * @param progress_callback An optional callback function for reporting progress.
	 */
	void runEventDrivenSim(World* world, pool::ThreadPool& pool,
	                       const std::function<void(const std::string&, int, int)>& progress_callback);
}
