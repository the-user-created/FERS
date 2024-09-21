// coord.h
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef COORD_H
#define COORD_H
#include "geometry_ops.h"


namespace math
{
	struct Coord
	{
		Vec3 pos;
		RealType t;

		bool operator<(const Coord& b) const { return t < b.t; }

		Coord& operator=(RealType a)
		{
			t = a;
			pos = {a, a, a};
			return *this;
		}
	};

	inline Coord operator*(const Coord& a, const Coord& b) { return {a.pos * b.pos, a.t}; }

	inline Coord operator+(const Coord& a, const Coord& b) { return {a.pos + b.pos, a.t}; }

	inline Coord operator-(const Coord& a, const Coord& b) { return {a.pos - b.pos, a.t}; }

	inline Coord operator/(const Coord& a, const Coord& b) { return {a.pos / b.pos, a.t}; }

	inline Coord operator+(const Coord& a, const RealType b) { return {a.pos + b, a.t}; }

	inline Coord operator*(const Coord& a, const RealType b) { return {a.pos * b, a.t}; }

	inline Coord operator/(const RealType a, const Coord& b) { return {a / b.pos, b.t}; }

	inline Coord operator/(const Coord& b, const RealType a) { return {b.pos / a, b.t}; }

	struct RotationCoord
	{
		RealType azimuth;
		RealType elevation;
		RealType t;

		bool operator<(const RotationCoord b) const { return t < b.t; }

		RotationCoord& operator=(const RealType a)
		{
			azimuth = elevation = t = a;
			return *this;
		}

		RotationCoord(const RealType a = 0) : azimuth(a), elevation(a), t(a) {} // NOLINT

		RotationCoord(const RealType az, const RealType el,
		              const RealType time) : azimuth(az), elevation(el), t(time) {}
	};

	inline RotationCoord operator*(const RotationCoord& a, const RotationCoord& b)
	{
		return {a.azimuth * b.azimuth, a.elevation * b.elevation, a.t};
	}

	inline RotationCoord operator+(const RotationCoord& a, const RotationCoord& b)
	{
		return {a.azimuth + b.azimuth, a.elevation + b.elevation, a.t};
	}

	inline RotationCoord operator-(const RotationCoord& a, const RotationCoord& b)
	{
		return {a.azimuth - b.azimuth, a.elevation - b.elevation, a.t};
	}

	inline RotationCoord operator/(const RotationCoord& a, const RotationCoord& b)
	{
		return {a.azimuth / b.azimuth, a.elevation / b.elevation, a.t};
	}

	inline RotationCoord operator+(const RotationCoord& a, const RealType b)
	{
		return {a.azimuth + b, a.elevation + b, a.t};
	}

	inline RotationCoord operator*(const RotationCoord& a, const RealType b)
	{
		return {a.azimuth * b, a.elevation * b, a.t};
	}

	inline RotationCoord operator/(const RealType a, const RotationCoord& b)
	{
		return {a / b.azimuth, a / b.elevation, b.t};
	}

	inline RotationCoord operator/(const RotationCoord& b, const RealType a)
	{
		return {b.azimuth / a, b.elevation / a, b.t};
	}
}

#endif //COORD_H
