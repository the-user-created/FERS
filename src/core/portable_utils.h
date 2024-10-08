/**
 * @file portable_utils.h
 * @brief Utility functions for mathematical and system operations.
 *
 * This file contains declarations and definitions of utility functions
 * that perform non-standard operations, such as computing the Bessel function
 * of the first kind and detecting the number of available CPUs on a machine.
 * The code uses C++20 features and provides safe, efficient implementations.
 *
 * @authors David Young, Marc Brooker
 * @date 2006-05-29
 */

#pragma once

#include <cmath>
#include <thread>

#include "config.h"

namespace core
{
	/**
	 * @brief Computes the Bessel function of the first kind (order 1) for a given value.
	 *
	 * This function calculates the Bessel function of the first kind,
	 * specifically of order 1, using the C++20 standard library function `std::cyl_bessel_j`.
	 * It is used in various scientific and engineering computations that involve
	 * wave propagation, signal processing, and more.
	 *
	 * @param x The value for which the Bessel function is to be computed.
	 * @return The computed value of the Bessel function of the first kind (order 1).
	 */
	inline RealType besselJ1(const RealType x) noexcept { return std::cyl_bessel_j(1, x); }

	/**
	 * @brief Detects the number of CPUs in the machine.
	 *
	 * This function attempts to detect the number of hardware threads (CPUs)
	 * available on the current machine using `std::thread::hardware_concurrency()`.
	 * If detection fails, an error is logged and 1 is returned as a default value,
	 * assuming the system has at least one CPU. This function is useful for
	 * multi-threading and system resource management.
	 *
	 * @return The number of CPUs detected, or 1 if detection fails.
	 */
	inline unsigned countProcessors() noexcept
	{
		if (const unsigned hardware_threads = std::thread::hardware_concurrency(); hardware_threads > 0)
		{
			return hardware_threads;
		}
		LOG(logging::Level::ERROR, "Unable to get CPU count, assuming 1.");
		return 1;
	}
}
