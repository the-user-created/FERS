/**
 * @file multipath_surface.cpp
 * @brief Implementation of multipath propagation
 *
 * @authors David Young, Marc Brooker
 * @date 2007-09-09
 */

#include "multipath_surface.h"

#include "math/geometry_ops.h"

namespace math
{
	MultipathSurface::MultipathSurface(const RealType a, const RealType b, const RealType c, const RealType d,
	                                   const RealType factor) noexcept
		: _factor(factor), _norm_factor(1 / (a * a + b * b + c * c)),
		  _translation_vector{-2 * a * d, -2 * b * d, -2 * c * d}
	{
		// Initialize _reflection matrix directly using structured bindings
		const auto mat = _reflection.getData(); // Assuming this returns a reference to the underlying array
		mat[0] = -a * a + b * b + c * c;
		mat[4] = a * a - b * b + c * c;
		mat[8] = a * a + b * b - c * c;

		// Symmetric components
		mat[1] = mat[3] = -2 * a * b;
		mat[2] = mat[6] = -2 * a * c;
		mat[5] = mat[7] = -2 * b * c;
	}

	Vec3 MultipathSurface::reflectPoint(const Vec3& b) const noexcept
	{
		Vec3 ans = b;
		ans *= _reflection; // Matrix multiplication
		ans -= _translation_vector;
		ans *= _norm_factor;
		return ans;
	}
}
