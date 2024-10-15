/**
* @file antenna_pattern.h
 * @brief Header file for antenna gain patterns.
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
	 */
	class Pattern
	{
	public:
		/**
		 * @brief Constructs a `Pattern` object.
		 *
		 * @param filename The name of the file containing the antenna gain pattern data.
		 */
		explicit Pattern(const std::string& filename)
			: _pattern(serial::readPattern(filename, "antenna")) {}

		~Pattern() = default;

		Pattern(const Pattern&) = default;
		Pattern(Pattern&&) = default;
		Pattern& operator=(const Pattern&) = default;
		Pattern& operator=(Pattern&&) = default;

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
		std::vector<std::vector<RealType>> _pattern; ///< The 2D pattern data.
	};
}
