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
	 * @brief Generates a white Gaussian noise (WGN) sample.
	 *
	 * @param rngEngine The random number generator engine to use.
	 * @param stddev The standard deviation of the WGN sample.
	 * @return A white Gaussian noise sample scaled by the given standard deviation.
	 */
	RealType wgnSample(std::mt19937& rngEngine, RealType stddev) noexcept;

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
}
