/**
 * @file noise_utils.h
 * @brief Utility functions for generating noise and calculating noise power.
 *
 * This file contains utility functions related to noise generation and noise calculations.
 * It includes functions to generate white Gaussian noise (WGN) samples and to convert noise temperature into power.
 * The file is intended to support various noise simulation and signal processing tasks in the system.
 *
 * @author David Young
 * @date 2024-09-17
 */

#pragma once

#include "config.h"
#include "core/parameters.h"

namespace noise
{
	/**
	 * @brief Generates a white Gaussian noise (WGN) sample.
	 *
	 * This function generates a sample of white Gaussian noise (WGN) with a specified standard deviation.
	 * The noise is drawn from a normal distribution with a mean of 0.
	 * It ensures the random number generator (RNG) is properly initialized.
	 *
	 * @param stddev The standard deviation of the WGN sample.
	 * @return A white Gaussian noise sample scaled by the given standard deviation.
	 */
	RealType wgnSample(RealType stddev) noexcept;

	/**
	 * @brief Converts noise temperature to noise power.
	 *
	 * This inline function calculates the noise power corresponding to a given noise temperature and bandwidth.
	 * The calculation is based on the Boltzmann constant.
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
