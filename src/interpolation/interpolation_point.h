/**
 * @file interpolation_point.h
 * @brief Defines a structure to store interpolation point data for signal processing.
 *
 * This file contains the declaration of the `InterpPoint` structure,
 * which is used to hold various signal processing parameters like power,
 * time, delay, doppler shift, phase, and noise temperature.
 * These points can be used in interpolation algorithms in signal processing tasks.
 *
 * @author David Young
 * @date 2024-09-12
 */

#pragma once

namespace interp
{
	/**
	 * @struct InterpPoint
	 * @brief Stores data for an interpolation point.
	 *
	 * This structure contains parameters that describe a single point in signal processing, including power, time, delay,
	 * doppler shift, phase, and noise temperature.
	 * These values are used in various interpolation algorithms for tasks such as signal modulation,
	 * transmission, and reception in the presence of noise.
	 */
	struct InterpPoint
	{
		/**
		 * @brief Power level of the signal at the interpolation point.
		 */
		RealType power;

		/**
		 * @brief Time at which the interpolation point is recorded.
		 */
		RealType time;

		/**
		 * @brief Delay associated with the signal at the interpolation point.
		 */
		RealType delay;

		/**
		 * @brief Doppler shift value at the interpolation point.
		 */
		RealType doppler;

		/**
		 * @brief Phase of the signal at the interpolation point.
		 */
		RealType phase;

		/**
		 * @brief Noise temperature at the interpolation point.
		 */
		RealType noise_temperature;
	};
}
