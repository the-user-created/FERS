// geometry_ops.h
// Classes for calculating the geometry of the world
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 26 May 2006

#ifndef GEOMETRY_OPS_H
#define GEOMETRY_OPS_H

#include <array>
#include <cmath>

#include "config.h"

namespace math
{
	class Vec3;

	class Matrix3
	{
	public:
		std::array<RealType, 9> elements{};

		Matrix3() = default;

		~Matrix3() = default;

		[[nodiscard]] const RealType* getData() const noexcept { return elements.data(); }
		RealType* getData() noexcept { return elements.data(); }
	};

	class SVec3
	{
	public:
		RealType length{}, azimuth{}, elevation{};

		constexpr SVec3() noexcept = default;

		constexpr SVec3(const RealType length, const RealType azimuth, const RealType elevation) noexcept
			: length(length), azimuth(azimuth), elevation(elevation) {}

		explicit SVec3(const Vec3& vec);

		~SVec3() = default;

		SVec3& operator*=(const RealType b) noexcept
		{
			length *= b;
			return *this;
		}

		SVec3& operator/=(const RealType b) noexcept
		{
			length /= b;
			return *this;
		}
	};

	class Vec3
	{
	public:
		RealType x{}, y{}, z{};

		constexpr Vec3() noexcept = default;

		constexpr Vec3(const RealType x, const RealType y, const RealType z) noexcept : x(x), y(y), z(z) {}

		explicit Vec3(const SVec3& svec) noexcept
			: x(svec.length * std::cos(svec.azimuth) * std::cos(svec.elevation)),
			  y(svec.length * std::sin(svec.azimuth) * std::cos(svec.elevation)),
			  z(svec.length * std::sin(svec.elevation)) {}

		Vec3& operator+=(const Vec3& b) noexcept
		{
			x += b.x;
			y += b.y;
			z += b.z;
			return *this;
		}

		Vec3& operator-=(const Vec3& b) noexcept
		{
			x -= b.x;
			y -= b.y;
			z -= b.z;
			return *this;
		}

		Vec3& operator*=(const Vec3& b) noexcept
		{
			x *= b.x;
			y *= b.y;
			z *= b.z;
			return *this;
		}

		Vec3& operator*=(const Matrix3& m) noexcept;

		Vec3& operator*=(const RealType b) noexcept
		{
			x *= b;
			y *= b;
			z *= b;
			return *this;
		}

		Vec3& operator/=(const RealType b) noexcept
		{
			x /= b;
			y /= b;
			z /= b;
			return *this;
		}

		Vec3 operator+(const RealType value) const { return {x + value, y + value, z + value}; }

		[[nodiscard]] RealType length() const noexcept { return std::sqrt(x * x + y * y + z * z); }
	};

	// Dot product
	inline RealType dotProduct(const Vec3& a, const Vec3& b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z; }

	// Cross product
	inline Vec3 crossProduct(const Vec3& a, const Vec3& b) noexcept
	{
		return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
	}

	// Operator overloads for Vec3
	inline Vec3 operator*(const Vec3& a, const Vec3& b) noexcept { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
	inline Vec3 operator+(const Vec3& a, const Vec3& b) noexcept { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
	inline Vec3 operator-(const Vec3& a, const Vec3& b) noexcept { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
	inline Vec3 operator/(const Vec3& a, const Vec3& b) { return {a.x / b.x, a.y / b.y, a.z / b.z}; }
	inline Vec3 operator*(const Vec3& a, const RealType b) noexcept { return {a.x * b, a.y * b, a.z * b}; }
	inline Vec3 operator/(const Vec3& a, const RealType b) noexcept { return {a.x / b, a.y / b, a.z / b}; }
	inline Vec3 operator/(const RealType a, const Vec3& b) noexcept { return {a / b.x, a / b.y, a / b.z}; }

	// SVec3 operator overloads
	inline SVec3 operator+(const SVec3& a, const SVec3& b) noexcept
	{
		RealType new_azimuth = fmod(a.azimuth + b.azimuth, 2 * PI);
		if (new_azimuth < 0) { new_azimuth += 2 * PI; }
		RealType new_elevation = fmod(a.elevation + b.elevation, PI);
		return {a.length + b.length, new_azimuth, new_elevation};
	}

	inline SVec3 operator-(const SVec3& a, const SVec3& b) noexcept
	{
		RealType new_azimuth = fmod(a.azimuth - b.azimuth, 2 * PI);
		if (new_azimuth < 0) { new_azimuth += 2 * PI; }
		RealType new_elevation = fmod(a.elevation - b.elevation, PI);
		return {a.length - b.length, new_azimuth, new_elevation};
	}
}

#endif
