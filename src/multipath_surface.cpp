// multipath_surface.cpp
// Implementation of multipath propagation
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 9 September 2007

#include "multipath_surface.h"

using namespace rs;

MultipathSurface::MultipathSurface(const RS_FLOAT a, const RS_FLOAT b, const RS_FLOAT c, const RS_FLOAT d,
                                   const RS_FLOAT factor)
	: _factor(factor)
{
	RS_FLOAT* mat = _reflection.getData();
	mat[0] = -a * a + b * b + c * c;
	mat[4] = a * a - b * b + c * c;
	mat[8] = a * a + b * b - c * c;
	mat[1] = mat[3] = -2 * a * b;
	mat[2] = mat[6] = -2 * a * c;
	mat[5] = mat[7] = -2 * b * c;
	_norm_factor = 1 / (a * a + b * b + c * c);
	_translation_vector = Vec3(-2 * a * d, -2 * b * d, -2 * c * d);
}

Vec3 MultipathSurface::reflectPoint(const Vec3& b) const
{
	Vec3 ans = b;
	ans *= _reflection;
	ans -= _translation_vector;
	ans *= _norm_factor;
	return ans;
}
