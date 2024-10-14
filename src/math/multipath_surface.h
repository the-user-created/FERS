/**
* @file multipath_surface.h
* @brief Classes and definitions for multipath propagation.
*
* @authors David Young, Marc Brooker
* @date 2007-09-09
*/

#pragma once

#include "config.h"
#include "geometry_ops.h"

namespace math
{
	/**
	 * @class MultipathSurface
	 * @brief A class representing a multipath surface.
	 */
	class MultipathSurface
	{
	public:
		/**
		 * @brief Constructor for MultipathSurface.
		 *
		 * @param a The first parameter of the surface.
		 * @param b The second parameter of the surface.
		 * @param c The third parameter of the surface.
		 * @param d The fourth parameter of the surface.
		 * @param factor The factor of the surface.
		 */
		MultipathSurface(RealType a, RealType b, RealType c, RealType d, RealType factor) noexcept;

		~MultipathSurface() = default;

		/**
		 * @brief Reflect a point on the multipath surface.
		 *
		 * @param b The point to reflect.
		 * @return The reflected point.
		 */
		[[nodiscard]] Vec3 reflectPoint(const Vec3& b) const noexcept;

		/**
		 * @brief Get the factor of the multipath surface.
		 *
		 * @return The factor of the multipath surface.
		 */
		[[nodiscard]] RealType getFactor() const noexcept { return _factor; }

	private:
		RealType _factor; ///< The factor of the multipath surface
		Matrix3 _reflection; ///< The reflection matrix of the multipath surface
		RealType _norm_factor; ///< The normalization factor of the multipath surface
		Vec3 _translation_vector; ///< The translation vector of the multipath surface
	};
}
