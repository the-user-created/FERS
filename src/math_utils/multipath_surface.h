// multipath_surface.h
// Classes and definitions for multipath propagation
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 9 September 2007

#ifndef MULTIPATH_SURFACE_H
#define MULTIPATH_SURFACE_H

#include "config.h"
#include "geometry_ops.h"

namespace math
{

	class MultipathSurface
	{
	public:
		MultipathSurface(RealType a, RealType b, RealType c, RealType d, RealType factor);

		~MultipathSurface() = default;

		[[nodiscard]] Vec3 reflectPoint(const Vec3& b) const;

		[[nodiscard]] RealType getFactor() const { return _factor; }

	private:
		RealType _factor;
		Matrix3 _reflection;
		RealType _norm_factor;
		Vec3 _translation_vector;
	};
}

#endif
