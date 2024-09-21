// geometry_ops.cpp
// Implementation of geometry classes
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 26 May 2006

#include "geometry_ops.h"

namespace math
{
	// Multiplication by a 3x3 matrix
	Vec3& Vec3::operator*=(const Matrix3& m)
	{
		const RS_FLOAT* mat = m.getData();
		const Vec3 v(x, y, z);
		x = mat[0] * v.x + mat[1] * v.y + mat[2] * v.z;
		y = mat[3] * v.x + mat[4] * v.y + mat[5] * v.z;
		z = mat[6] * v.x + mat[7] * v.y + mat[8] * v.z;
		return *this;
	}

	// Constructor with a rectangular vector
	SVec3::SVec3(const Vec3& vec)
	{
		length = vec.length();
		if (length != 0)
		{
			elevation = std::asin(vec.z / length);
			azimuth = std::atan2(vec.y, vec.x);
		}
		else
		{
			elevation = 0;
			azimuth = 0;
		}
	}
}
