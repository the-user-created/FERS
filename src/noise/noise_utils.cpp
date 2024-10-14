/**
 * @file noise_utils.cpp
 * @brief Utility functions for generating noise samples.
 *
 * @author David Young
 * @date 2024-09-17
 */

#include "noise_utils.h"

#include <optional>
#include <random>

#include "core/parameters.h"

namespace
{
	/**
	 * @brief Normal distribution for generating white Gaussian noise samples.
	 */
	std::normal_distribution normal_dist{0.0, 1.0};

	/**
	 * @brief Get the random seed for the noise generator.
	 *
	 * @return The random seed for the noise generator.
	 */
	unsigned int getSeed() noexcept
	{
		const unsigned int seed = params::randomSeed();
		return seed != 0 ? seed : std::random_device{}();
	}
}

namespace noise
{
	RealType wgnSample(const RealType stddev) noexcept
	{
		if (stddev <= EPSILON) { return 0.0; }

		ensureInitialized();
		return normal_dist(*rng) * stddev;
	}

	void ensureInitialized() noexcept { if (!rng) { rng.emplace(getSeed()); } }
}
