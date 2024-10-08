/**
* @file geometry_ops.cpp
* @brief Implementation of geometry classes.
*
* This file implements the Vec3, SVec3, and Matrix3 classes for handling 3D vector and matrix operations.
*
* @authors David Young, Marc Brooker
* @date 2006-05-26
*/

#include "geometry_ops.h"

namespace math
{
	Vec3::Vec3(const SVec3& svec) noexcept : x(svec.length * std::cos(svec.azimuth) * std::cos(svec.elevation)),
	                                         y(svec.length * std::sin(svec.azimuth) * std::cos(svec.elevation)),
	                                         z(svec.length * std::sin(svec.elevation)) {}

	Vec3& Vec3::operator+=(const Vec3& b) noexcept
	{
		x += b.x;
		y += b.y;
		z += b.z;
		return *this;
	}

	Vec3& Vec3::operator-=(const Vec3& b) noexcept
	{
		x -= b.x;
		y -= b.y;
		z -= b.z;
		return *this;
	}

	Vec3& Vec3::operator*=(const Vec3& b) noexcept
	{
		x *= b.x;
		y *= b.y;
		z *= b.z;
		return *this;
	}

	// Multiplication by a 3x3 matrix
	Vec3& Vec3::operator*=(const Matrix3& m) noexcept
	{
		const RealType* mat = m.getData();
		const Vec3 v(x, y, z);
		x = mat[0] * v.x + mat[1] * v.y + mat[2] * v.z;
		y = mat[3] * v.x + mat[4] * v.y + mat[5] * v.z;
		z = mat[6] * v.x + mat[7] * v.y + mat[8] * v.z;
		return *this;
	}

	Vec3& Vec3::operator*=(const RealType b) noexcept
	{
		x *= b;
		y *= b;
		z *= b;
		return *this;
	}

	Vec3& Vec3::operator/=(const RealType b) noexcept
	{
		x /= b;
		y /= b;
		z /= b;
		return *this;
	}

	// Constructor with a rectangular vector
	SVec3::SVec3(const Vec3& vec) noexcept : length(vec.length())
	{
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

	SVec3& SVec3::operator*=(const RealType b) noexcept
	{
		length *= b;
		return *this;
	}

	SVec3& SVec3::operator/=(const RealType b) noexcept
	{
		length /= b;
		return *this;
	}

	SVec3 operator+(const SVec3& a, const SVec3& b) noexcept
	{
		RealType new_azimuth = fmod(a.azimuth + b.azimuth, 2 * PI);
		if (new_azimuth < 0) { new_azimuth += 2 * PI; }
		RealType new_elevation = fmod(a.elevation + b.elevation, PI);
		return {a.length + b.length, new_azimuth, new_elevation};
	}

	SVec3 operator-(const SVec3& a, const SVec3& b) noexcept
	{
		RealType new_azimuth = fmod(a.azimuth - b.azimuth, 2 * PI);
		if (new_azimuth < 0) { new_azimuth += 2 * PI; }
		RealType new_elevation = fmod(a.elevation - b.elevation, PI);
		return {a.length - b.length, new_azimuth, new_elevation};
	}
}
