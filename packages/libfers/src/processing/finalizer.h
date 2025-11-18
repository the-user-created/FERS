// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
 * @file finalizer.h
 * @brief Declares the functions for the asynchronous receiver finalization pipelines.
 */

#pragma once

#include <memory>
#include <vector>

namespace radar
{
	class Receiver;

	class Target;
}

namespace pool
{
	class ThreadPool;
}

namespace processing
{
	/**
	 * @brief The main function for a dedicated pulsed-mode receiver finalizer thread.
	 *
	 * This function runs in a loop, dequeuing and processing RenderingJobs for a
	 * specific receiver. It handles all expensive rendering, signal processing,
	 * and I/O for that receiver's data.
	 *
	 * @param receiver A pointer to the pulsed-mode receiver to process.
	 * @param pool A pointer to the main thread pool for parallelizing sub-tasks.
	 * @param targets A pointer to the world's list of targets for interference calculation.
	 */
	void runPulsedFinalizer(radar::Receiver* receiver, pool::ThreadPool* pool,
							const std::vector<std::unique_ptr<radar::Target>>* targets);

	/**
	 * @brief The finalization task for a continuous-wave (CW) mode receiver.
	 *
	 * This function is submitted to the main thread pool when a CW receiver
	 * finishes its operation. It processes the entire collected I/Q buffer,
	 * applies interference and noise, and writes the final data to a file.
	 *
	 * @param receiver A pointer to the CW-mode receiver to finalize.
	 * @param pool A pointer to the main thread pool for parallelizing sub-tasks.
	 */
	void finalizeCwReceiver(radar::Receiver* receiver, pool::ThreadPool* pool);
}
