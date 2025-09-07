/**
 * @file noise_utils.cpp
 * @brief Utility functions for generating noise samples.
 *
 * @author David Young
 * @date 2024-09-17
 */

#include "noise_utils.h"

#include <random>

#include "core/parameters.h"

namespace noise
{
	RealType wgnSample(std::mt19937& rngEngine, const RealType stddev) noexcept
	{
		if (stddev <= EPSILON) { return 0.0; }

		// Use a thread-local distribution to avoid contention on a single global object
		thread_local std::normal_distribution normal_dist{0.0, 1.0};
		return normal_dist(rngEngine) * stddev;
	}
}
