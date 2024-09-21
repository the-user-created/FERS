// portable_utils.h
// Declarations for functions containing non-standard code
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 29 May 2006

#ifndef PORTABLE_UTILS_H
#define PORTABLE_UTILS_H

#include <cmath>
#include <thread>

#include "config.h"

namespace core
{
	/**
	 * \brief Computes the Bessel function of the first kind (order 1) for a given value.
	 *
	 * This function uses the standardized C++20 <cmath> library function std::cyl_bessel_j.
	 *
	 * \param x The value for which the Bessel function is to be computed.
	 * \return The computed value of the Bessel function of the first kind (order 1).
	 */
	inline RS_FLOAT besselJ1(const RS_FLOAT x)
	{
		return std::cyl_bessel_j(1, x);
	}

	/**
	 * \brief Detects the number of CPUs in the machine.
	 *
	 * This function attempts to detect the number of hardware threads (CPUs) available on the machine.
	 * If the detection fails, it logs an error and returns 1 as a fallback.
	 *
	 * \return The number of CPUs detected, or 1 if detection fails.
	 */
	inline unsigned countProcessors()
	{
		if (const unsigned hardware_threads = std::thread::hardware_concurrency(); hardware_threads > 0)
		{
			return hardware_threads;
		}
		LOG(logging::Level::ERROR, "Unable to get CPU count, assuming 1.");
		return 1;
	}
}

#endif
