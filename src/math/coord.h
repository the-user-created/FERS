// coord.h
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#pragma once

#include "geometry_ops.h"

namespace math
{
	struct Coord
	{
		Vec3 pos;
		RealType t;

		bool operator<(const Coord& b) const noexcept { return t < b.t; }

		Coord& operator=(RealType a) noexcept
		{
			t = a;
			pos = {a, a, a};
			return *this;
		}
	};

	inline Coord operator*(const Coord& a, const Coord& b) noexcept { return {a.pos * b.pos, a.t}; }
	inline Coord operator+(const Coord& a, const Coord& b) noexcept { return {a.pos + b.pos, a.t}; }
	inline Coord operator-(const Coord& a, const Coord& b) noexcept { return {a.pos - b.pos, a.t}; }
	inline Coord operator/(const Coord& a, const Coord& b) noexcept { return {a.pos / b.pos, a.t}; }
	inline Coord operator+(const Coord& a, const RealType b) noexcept { return {a.pos + b, a.t}; }
	inline Coord operator*(const Coord& a, const RealType b) noexcept { return {a.pos * b, a.t}; }
	inline Coord operator/(const RealType a, const Coord& b) noexcept { return {a / b.pos, b.t}; }
	inline Coord operator/(const Coord& b, const RealType a) noexcept { return {b.pos / a, b.t}; }

	struct RotationCoord
	{
		RealType azimuth{}, elevation{}, t{};

		bool operator<(const RotationCoord& b) const noexcept { return t < b.t; }

		RotationCoord& operator=(const RealType a) noexcept
		{
			azimuth = elevation = t = a;
			return *this;
		}

		constexpr explicit RotationCoord(const RealType a = 0) noexcept : azimuth(a), elevation(a), t(a) {}

		RotationCoord(const RealType az, const RealType el, const RealType time) noexcept
			: azimuth(az), elevation(el), t(time) {}
	};

	inline RotationCoord operator*(const RotationCoord& a, const RotationCoord& b) noexcept
	{
		return {a.azimuth * b.azimuth, a.elevation * b.elevation, a.t};
	}

	inline RotationCoord operator+(const RotationCoord& a, const RotationCoord& b) noexcept
	{
		return {a.azimuth + b.azimuth, a.elevation + b.elevation, a.t};
	}

	inline RotationCoord operator-(const RotationCoord& a, const RotationCoord& b) noexcept
	{
		return {a.azimuth - b.azimuth, a.elevation - b.elevation, a.t};
	}

	inline RotationCoord operator/(const RotationCoord& a, const RotationCoord& b) noexcept
	{
		return {a.azimuth / b.azimuth, a.elevation / b.elevation, a.t};
	}

	inline RotationCoord operator+(const RotationCoord& a, const RealType b) noexcept
	{
		return {a.azimuth + b, a.elevation + b, a.t};
	}

	inline RotationCoord operator*(const RotationCoord& a, const RealType b) noexcept
	{
		return {a.azimuth * b, a.elevation * b, a.t};
	}

	inline RotationCoord operator/(const RealType a, const RotationCoord& b) noexcept
	{
		return {a / b.azimuth, a / b.elevation, b.t};
	}

	inline RotationCoord operator/(const RotationCoord& b, const RealType a) noexcept
	{
		return {b.azimuth / a, b.elevation / a, b.t};
	}
}
