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
	 * @brief Global random engine wrapped in std::optional for lazy initialization.
	 */
	std::optional<std::mt19937> rng;

	/**
	 * @brief Normal distribution for generating white Gaussian noise samples.
	 */
	std::normal_distribution normal_dist{0.0, 1.0};

	/**
	 * @brief Get the random seed for the noise generator.
	 *
	 * This function retrieves the random seed from the global parameters.
	 * If the seed is set to 0, it generates a new seed using std::random_device.
	 *
	 * @return The random seed for the noise generator.
	 */
	unsigned int getSeed() noexcept
	{
		const unsigned int seed = params::randomSeed();
		return seed != 0 ? seed : std::random_device{}();
	}

	/**
	 * @brief Ensure the random number generator (RNG) is initialized.
	 *
	 * This function initializes the RNG lazily when it is first accessed.
	 */
	void ensureInitialized() noexcept { if (!rng) { rng.emplace(getSeed()); } }
}

namespace noise
{
	// Generate a white Gaussian noise sample with the specified standard deviation
	RealType wgnSample(const RealType stddev) noexcept
	{
		if (stddev <= EPSILON) { return 0.0; }

		ensureInitialized(); // Ensure RNG is initialized
		return normal_dist(*rng) * stddev;
	}
}
