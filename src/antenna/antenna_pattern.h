/**
* @file antenna_pattern.h
 * @brief Header file for antenna gain pattern interpolation.
 *
 * This file contains the declaration of the `Pattern` class, which is used to handle antenna gain patterns.
 * It includes methods for loading gain patterns from a file and calculating the interpolated gain
 * at specific angles using a bilinear interpolation technique.
 * The gain patterns are represented as 2D arrays.
 *
 * @authors David Young, Marc Brooker
 * @date 2007-09-11
 */

#pragma once

#include <string>
#include <vector>

#include "config.h"

namespace serial
{
	std::vector<std::vector<RealType>> readPattern(const std::string& name, const std::string& datasetName);
}

namespace math
{
	class SVec3;
}

namespace antenna
{
	/**
	 * @class Pattern
	 * @brief Represents an antenna gain pattern.
	 *
	 * The `Pattern` class is responsible for loading and handling the antenna gain pattern stored in a 2D array.
	 * It provides a method to calculate the interpolated gain for a given azimuth and elevation angle.
	 */
	class Pattern
	{
	public:
		/**
		 * @brief Constructs a `Pattern` object.
		 *
		 * This constructor loads the gain pattern from a file using the `readPattern` function.
		 *
		 * @param filename The name of the file containing the antenna gain pattern data.
		 */
		explicit Pattern(const std::string& filename)
			: _pattern(serial::readPattern(filename, "antenna")) {}

		/**
		 * @brief Default destructor.
		 *
		 * The default destructor for the `Pattern` class.
		 */
		~Pattern() = default;

		/**
		 * @brief Calculates the gain for a given angle.
		 *
		 * This function takes an angle (azimuth and elevation) and returns the interpolated gain value
		 * from the stored 2D pattern data using bilinear interpolation.
		 *
		 * @param angle The azimuth and elevation angle (represented by `SVec3`).
		 * @return The interpolated gain value.
		 */
		[[nodiscard]] RealType getGain(const math::SVec3& angle) const noexcept;

	private:
		/**
		 * @brief The 2D array representing the antenna gain pattern.
		 */
		std::vector<std::vector<RealType>> _pattern;
	};
}
