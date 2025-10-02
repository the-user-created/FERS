// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
* @file sim_threading.h
* @brief Header file for threaded simulation management.
*
* This file contains the definitions and classes necessary for running a threaded simulation.
* It includes classes that handle simulations for transmitter-receiver pairs and rendering processes for each receiver.
*/

#pragma once

#include <exception>
#include <functional>

#include <libfers/config.h>

namespace pool
{
	class ThreadPool;
}

namespace radar
{
	class Receiver;

	class Transmitter;
}

namespace core
{
	class World;

	/**
     * @struct ReResults
     * @brief Stores the results of a radar simulation.
     */
	struct ReResults
	{
		RealType power; /**< Power of the radar signal. */
		RealType delay; /**< Signal delay in time. */
		RealType doppler_factor; /**< Doppler factor of the radar signal (f_recv/f_trans). */
		RealType phase; /**< Phase of the radar signal. */
		RealType noise_temperature; /**< Noise temperature affecting the radar signal. */
	};

	/**
	 * @class RangeError
	 * @brief Exception class for range calculation errors.
	 */
	class RangeError final : public std::exception
	{
	public:
		/**
		 * @brief Provides the error message for the exception.
		 *
		 * @return The error message string.
		 */
		[[nodiscard]] const char* what() const noexcept override { return "Range error in RE calculations"; }
	};

	void runThreadedSim(const World* world, pool::ThreadPool& pool);

	void runThreadedCwSim(const World* world, pool::ThreadPool& pool);
}
