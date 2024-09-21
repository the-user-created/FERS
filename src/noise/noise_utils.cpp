//
// Created by David Young on 9/17/24.
//

#include "noise_utils.h"

#include <memory>
#include <random>

#include "core/parameters.h"

namespace
{
	// Use the Mersenne Twister PRNG with parameter 19937 from the C++ standard library
	std::unique_ptr<std::mt19937> rng;
	std::unique_ptr<std::normal_distribution<>> normal_dist;
	std::unique_ptr<std::uniform_real_distribution<>> uniform_dist;

	// Seed generation can be updated with a random device if no explicit seed is provided
	unsigned int getSeed()
	{
		return params::randomSeed(); // Assuming this function still provides a seed, else use random_device
	}
}

// =================================================================================================================
//
// IMPLEMENTATIONS OF NON-CLASS FUNCTIONS
//
// =================================================================================================================

namespace noise
{
	void initializeNoise()
	{
		// Initialize random number generator and distributions using smart pointers
		rng = std::make_unique<std::mt19937>(getSeed());
		normal_dist = std::make_unique<std::normal_distribution<>>(0.0, 1.0);
		uniform_dist = std::make_unique<std::uniform_real_distribution<>>(0.0, 1.0);
	}

	void cleanUpNoise()
	{
		// Smart pointers automatically clean up, no need for manual deletion
		rng.reset();
		normal_dist.reset();
		uniform_dist.reset();
	}

	RS_FLOAT wgnSample(const double stddev)
	{
		// Generate a white Gaussian noise sample with the specified standard deviation
		return stddev > std::numeric_limits<double>::epsilon() ? (*normal_dist)(*rng) * stddev : 0.0;
	}

	RS_FLOAT uniformSample()
	{
		// Generate a sample from a uniform distribution between 0 and 1
		return (*uniform_dist)(*rng);
	}

	RS_FLOAT noiseTemperatureToPower(const double temperature, const double bandwidth)
	{
		// Compute noise power given temperature and bandwidth
		return params::boltzmannK() * temperature * bandwidth;
	}
}
