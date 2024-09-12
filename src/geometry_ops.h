// geometry_ops.h
// Classes for calculating the geometry of the world
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 26 May 2006

#ifndef GEOMETRY_OPS_H
#define GEOMETRY_OPS_H

#include <cmath>

#include "config.h"

namespace rs
{
	class SVec3;

	class Matrix3
	{
	public:
		RS_FLOAT elements[9]{};

		Matrix3() = default;

		~Matrix3() = default;

		[[nodiscard]] const RS_FLOAT* getData() const
		{
			return elements;
		}

		RS_FLOAT* getData()
		{
			return elements;
		}
	};

	class Vec3
	{
	public:
		RS_FLOAT x, y, z;

		Vec3() : x(0), y(0), z(0)
		{
		}

		Vec3(const RS_FLOAT x, const RS_FLOAT y, const RS_FLOAT z) : x(x), y(y), z(z)
		{
		}

		explicit Vec3(const SVec3& svec);

		~Vec3() = default;

		Vec3& operator+=(const Vec3& b);

		Vec3& operator-=(const Vec3& b);

		Vec3& operator*=(const Vec3& b);

		Vec3& operator=(const Vec3& b);

		Vec3& operator*=(const Matrix3& m);

		Vec3& operator*=(RS_FLOAT b);

		Vec3& operator/=(RS_FLOAT b);

		Vec3& operator+=(RS_FLOAT b);

		[[nodiscard]] RS_FLOAT length() const;
	};

	RS_FLOAT dotProduct(const Vec3& a, const Vec3& b);

	Vec3 crossProduct(const Vec3& a, const Vec3& b);

	Vec3 operator*(const Vec3& a, const Vec3& b);

	Vec3 operator+(const Vec3& a, const Vec3& b);

	Vec3 operator-(const Vec3& a, const Vec3& b);

	Vec3 operator/(const Vec3& a, const Vec3& b);

	Vec3 operator*(const Vec3& a, RS_FLOAT b);

	Vec3 operator/(const Vec3& a, RS_FLOAT b);

	Vec3 operator/(RS_FLOAT a, const Vec3& b);

	class SVec3
	{
	public:
		RS_FLOAT length, azimuth, elevation;

		SVec3() : length(0), azimuth(0), elevation(0)
		{
		}

		SVec3(const RS_FLOAT length, const RS_FLOAT azimuth, const RS_FLOAT elevation)
			: length(length), azimuth(azimuth), elevation(elevation)
		{
		}

		SVec3(const SVec3& svec) = default;

		explicit SVec3(const Vec3& vec);

		~SVec3() = default;

		SVec3& operator*=(RS_FLOAT b);

		SVec3& operator/=(RS_FLOAT b);
	};

	inline SVec3 operator+(const SVec3& a, const SVec3& b)
	{
		RS_FLOAT new_azimuth = fmod(a.azimuth + b.azimuth, 2 * M_PI);
		if (new_azimuth < 0)
		{
			new_azimuth += 2 * M_PI;
		}
		RS_FLOAT new_elevation = fmod(a.elevation + b.elevation, M_PI);
		return {a.length + b.length, new_azimuth, new_elevation};
	}

	inline SVec3 operator-(const SVec3& a, const SVec3& b)
	{
		RS_FLOAT new_azimuth = fmod(a.azimuth - b.azimuth, 2 * M_PI);
		if (new_azimuth < 0)
		{
			new_azimuth += 2 * M_PI;
		}
		RS_FLOAT new_elevation = fmod(a.elevation - b.elevation, M_PI);
		return {a.length - b.length, new_azimuth, new_elevation};
	}
}

#endif
