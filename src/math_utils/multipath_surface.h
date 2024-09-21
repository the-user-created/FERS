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
		MultipathSurface(RS_FLOAT a, RS_FLOAT b, RS_FLOAT c, RS_FLOAT d, RS_FLOAT factor);

		~MultipathSurface() = default;

		[[nodiscard]] Vec3 reflectPoint(const Vec3& b) const;

		[[nodiscard]] RS_FLOAT getFactor() const { return _factor; }

	private:
		RS_FLOAT _factor;
		Matrix3 _reflection;
		RS_FLOAT _norm_factor;
		Vec3 _translation_vector;
	};
}

#endif
