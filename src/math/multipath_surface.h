// multipath_surface.h
// Classes and definitions for multipath propagation
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Modified by: David Young
// 9 September 2007

#pragma once

#include "config.h"
#include "geometry_ops.h"

namespace math
{
	class MultipathSurface
	{
	public:
		MultipathSurface(RealType a, RealType b, RealType c, RealType d, RealType factor) noexcept;

		~MultipathSurface() = default;

		[[nodiscard]] Vec3 reflectPoint(const Vec3& b) const noexcept;

		[[nodiscard]] RealType getFactor() const noexcept { return _factor; }

	private:
		RealType _factor;
		Matrix3 _reflection;
		RealType _norm_factor;
		Vec3 _translation_vector;
	};
}
