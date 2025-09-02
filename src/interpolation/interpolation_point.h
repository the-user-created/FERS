/**
 * @file interpolation_point.h
 * @brief Defines a structure to store interpolation point data for signal processing.
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
	 */
	struct InterpPoint
	{
		RealType power; ///< Power level of the signal at the interpolation point.
		RealType time; ///< Time at which the interpolation point is recorded.
		RealType delay; ///< Delay associated with the signal at the interpolation point.
		RealType doppler_factor; ///< Doppler factor (f_recv/f_trans) at the interpolation point.
		RealType phase; ///< Phase of the signal at the interpolation point.
		RealType noise_temperature; ///< Noise temperature at the interpolation point.
	};
}
