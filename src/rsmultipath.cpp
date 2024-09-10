//rsmultipath.cpp
//Implementation of multipath propagation
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//9 September 2007

#include "rsmultipath.h"

using namespace rs;

//
// MultipathSurface implementation
//

/// Constructor
MultipathSurface::MultipathSurface(const rsFloat a, const rsFloat b, const rsFloat c, const rsFloat d, const rsFloat factor):
	_factor(factor)
{
	rsFloat* mat = _reflection.getData();
	//Fill the reflection matrix
	mat[0] = -a * a + b * b + c * c;
	mat[4] = a * a - b * b + c * c;
	mat[8] = a * a + b * b - c * c;
	mat[1] = mat[3] = -2 * a * b;
	mat[2] = mat[6] = -2 * a * c;
	mat[5] = mat[7] = -2 * b * c;
	//Get the scale factor
	_norm_factor = 1 / (a * a + b * b + c * c);
	//Get the translation vector
	_translation_vector = Vec3(-2 * a * d, -2 * b * d, -2 * c * d);
}

/// Default destructor
MultipathSurface::~MultipathSurface()
{
}

/// Return a point reflected in the surface
Vec3 MultipathSurface::reflectPoint(const Vec3& b) const
{
	Vec3 ans = b;
	// Calculate the reflected position of b
	// Calculation is norm_factor*(reflection*b+translation_vector)
	ans *= _reflection;
	ans -= _translation_vector;
	ans *= _norm_factor;
	return ans;
}

/// Get the reflectance factor
rsFloat MultipathSurface::getFactor() const
{
	return _factor;
}
