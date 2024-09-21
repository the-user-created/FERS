// geometry_ops.h
// Classes for calculating the geometry of the world
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 26 May 2006

#ifndef GEOMETRY_OPS_H
#define GEOMETRY_OPS_H

#include <cmath>

#include "config.h"

namespace math
{
	class Vec3;

	class Matrix3
	{
	public:
		RealType elements[9]{};

		Matrix3() = default;

		~Matrix3() = default;

		[[nodiscard]] const RealType* getData() const { return elements; }

		RealType* getData() { return elements; }
	};

	class SVec3
	{
	public:
		RealType length, azimuth, elevation;

		SVec3() : length(0), azimuth(0), elevation(0) {}

		SVec3(const RealType length, const RealType azimuth, const RealType elevation)
			: length(length), azimuth(azimuth), elevation(elevation) {}

		SVec3(const SVec3& svec) = default;

		explicit SVec3(const Vec3& vec);

		~SVec3() = default;

		SVec3& operator*=(const RealType b)
		{
			length *= b;
			return *this;
		}

		SVec3& operator/=(const RealType b)
		{
			length /= b;
			return *this;
		}
	};

	class Vec3
	{
	public:
		RealType x, y, z;

		Vec3() : x(0), y(0), z(0) {}

		Vec3(const RealType x, const RealType y, const RealType z) : x(x), y(y), z(z) {}

		explicit Vec3(const SVec3& svec) : x(svec.length * std::cos(svec.azimuth) * std::cos(svec.elevation)),
		                                   y(svec.length * std::sin(svec.azimuth) * std::cos(svec.elevation)),
		                                   z(svec.length * std::sin(svec.elevation)) {}

		~Vec3() = default;

		Vec3& operator+=(const Vec3& b)
		{
			x += b.x;
			y += b.y;
			z += b.z;
			return *this;
		}

		Vec3& operator-=(const Vec3& b)
		{
			x -= b.x;
			y -= b.y;
			z -= b.z;
			return *this;
		}

		Vec3& operator*=(const Vec3& b)
		{
			x *= b.x;
			y *= b.y;
			z *= b.z;
			return *this;
		}

		Vec3& operator=(const Vec3& b) = default;

		Vec3& operator*=(const Matrix3& m);

		Vec3& operator*=(const RealType b)
		{
			x *= b;
			y *= b;
			z *= b;
			return *this;
		}

		Vec3& operator/=(const RealType b)
		{
			x /= b;
			y /= b;
			z /= b;
			return *this;
		}

		Vec3& operator+=(const RealType b)
		{
			x += b;
			y += b;
			z += b;
			return *this;
		}

		Vec3 operator+(const RealType value) const { return {x + value, y + value, z + value}; }

		[[nodiscard]] RealType length() const { return std::sqrt(x * x + y * y + z * z); }
	};

	inline RealType dotProduct(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

	// Note: This function is not used in the codebase
	inline Vec3 crossProduct(const Vec3& a, const Vec3& b)
	{
		return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
	}

	inline Vec3 operator*(const Vec3& a, const Vec3& b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }

	inline Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }

	inline Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }

	inline Vec3 operator/(const Vec3& a, const Vec3& b) { return {a.x / b.x, a.y / b.y, a.z / b.z}; }

	inline Vec3 operator*(const Vec3& a, const RealType b) { return {a.x * b, a.y * b, a.z * b}; }

	inline Vec3 operator/(const Vec3& a, const RealType b) { return {a.x / b, a.y / b, a.z / b}; }

	inline Vec3 operator/(const RealType a, const Vec3& b) { return {a / b.x, a / b.y, a / b.z}; }

	inline SVec3 operator+(const SVec3& a, const SVec3& b)
	{
		RealType new_azimuth = fmod(a.azimuth + b.azimuth, 2 * M_PI);
		if (new_azimuth < 0) { new_azimuth += 2 * M_PI; }
		RealType new_elevation = fmod(a.elevation + b.elevation, M_PI);
		return {a.length + b.length, new_azimuth, new_elevation};
	}

	inline SVec3 operator-(const SVec3& a, const SVec3& b)
	{
		RealType new_azimuth = fmod(a.azimuth - b.azimuth, 2 * M_PI);
		if (new_azimuth < 0) { new_azimuth += 2 * M_PI; }
		RealType new_elevation = fmod(a.elevation - b.elevation, M_PI);
		return {a.length - b.length, new_azimuth, new_elevation};
	}
}

#endif
