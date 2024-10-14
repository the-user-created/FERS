/**
 * @file noise_utils.h
 * @brief Utility functions for generating noise and calculating noise power.
 *
 * @author David Young
 * @date 2024-09-17
 */

#pragma once

#include <random>

#include "config.h"
#include "core/parameters.h"

namespace noise
{
	/**
	 * @brief Global random engine wrapped in std::optional for lazy initialization.
	 */
	inline std::optional<std::mt19937> rng;

	/**
	 * @brief Generates a white Gaussian noise (WGN) sample.
	 *
	 * @param stddev The standard deviation of the WGN sample.
	 * @return A white Gaussian noise sample scaled by the given standard deviation.
	 */
	RealType wgnSample(RealType stddev) noexcept;

	/**
	 * @brief Converts noise temperature to noise power.
	 *
	 * @param temperature The noise temperature in Kelvin.
	 * @param bandwidth The bandwidth over which the noise is measured, in Hertz.
	 * @return The noise power in watts.
	 */
	inline RealType noiseTemperatureToPower(const RealType temperature, const RealType bandwidth) noexcept
	{
		return params::boltzmannK() * temperature * bandwidth;
	}

	/**
	 * @brief Ensure the random number generator (RNG) is initialized.
	 */
	void ensureInitialized() noexcept;
}
