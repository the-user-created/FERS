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
 * This file contains the declarations for the high-level functions that
 * orchestrate and manage a multi-threaded radar simulation. It defines the main
 * entry points for running both pulsed and Continuous Wave (CW) simulations.
 */

#pragma once

#include <functional>
#include <string>

// Forward declarations
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
	 * @brief Runs a multi-threaded pulsed radar simulation.
	 *
	 * This function orchestrates the simulation of all transmitter-receiver pairs
	 * for a pulsed radar scenario. It uses a thread pool to parallelize the
	 * simulation of each pair and the subsequent rendering of receiver data.
	 *
	 * @param world A pointer to the simulation world containing all entities.
	 * @param pool A reference to the thread pool for executing tasks.
	 * @param progress_callback An optional callback function for reporting progress.
	 */
	void runThreadedSim(const World* world, pool::ThreadPool& pool,
	                    const std::function<void(const std::string&, int, int)>& progress_callback);

	/**
	 * @brief Runs a multi-threaded Continuous Wave (CW) radar simulation.
	 *
	 * This function orchestrates the simulation for a CW radar scenario. It divides
	 * the simulation time into discrete samples and uses a thread pool to calculate
	 * the I/Q value at each receiver for each time step.
	 *
	 * @param world A pointer to the simulation world containing all entities.
	 * @param pool A reference to the thread pool for executing tasks.
	 * @param progress_callback An optional callback function for reporting progress.
	 */
	void runThreadedCwSim(const World* world, pool::ThreadPool& pool,
	                      const std::function<void(const std::string&, int, int)>& progress_callback);
}
