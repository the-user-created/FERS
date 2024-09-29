// noise_utils.cpp
// Created by David Young on 9/17/24.
//

#include "noise_utils.h"

#include <limits>             // for numeric_limits
#include <optional>           // for optional
#include <random>             // for mersenne_twister_engine, normal_distrib...

#include "core/parameters.h"  // for boltzmannK, randomSeed

namespace
{
	// Global random engine wrapped in std::optional for lazy initialization
	std::optional<std::mt19937> rng;

	// Predefined normal and uniform distributions
	std::normal_distribution normal_dist{0.0, 1.0};
	std::uniform_real_distribution uniform_dist{0.0, 1.0};

	// Get the seed either from params or use a random_device for better entropy
	unsigned int getSeed()
	{
		const unsigned int seed = params::randomSeed();
		return seed != 0 ? seed : std::random_device{}();
	}

	// Ensure the RNG is initialized lazily
	void ensureInitialized() { if (!rng) { rng.emplace(getSeed()); } }
}

namespace noise
{
	// Generate a white Gaussian noise sample with the specified standard deviation
	RealType wgnSample(const RealType stddev)
	{
		if (stddev <= EPSILON) { return 0.0; }

		ensureInitialized(); // Ensure RNG is initialized
		return normal_dist(*rng) * stddev;
	}

	// Generate a sample from a uniform distribution between 0 and 1
	RealType uniformSample()
	{
		ensureInitialized(); // Ensure RNG is initialized
		return uniform_dist(*rng);
	}

	// Compute noise power given temperature and bandwidth
	RealType noiseTemperatureToPower(const RealType temperature, const RealType bandwidth)
	{
		return params::boltzmannK() * temperature * bandwidth;
	}
}
