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
		RS_FLOAT t;

		bool operator<(const Coord& b) const { return t < b.t; }

		Coord& operator=(RS_FLOAT a)
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

	inline Coord operator+(const Coord& a, const RS_FLOAT b) { return {a.pos + b, a.t}; }

	inline Coord operator*(const Coord& a, const RS_FLOAT b) { return {a.pos * b, a.t}; }

	inline Coord operator/(const RS_FLOAT a, const Coord& b) { return {a / b.pos, b.t}; }

	inline Coord operator/(const Coord& b, const RS_FLOAT a) { return {b.pos / a, b.t}; }

	struct RotationCoord
	{
		RS_FLOAT azimuth;
		RS_FLOAT elevation;
		RS_FLOAT t;

		bool operator<(const RotationCoord b) const { return t < b.t; }

		RotationCoord& operator=(const RS_FLOAT a)
		{
			azimuth = elevation = t = a;
			return *this;
		}

		RotationCoord(const RS_FLOAT a = 0) : azimuth(a), elevation(a), t(a) {} // NOLINT

		RotationCoord(const RS_FLOAT az, const RS_FLOAT el,
		              const RS_FLOAT time) : azimuth(az), elevation(el), t(time) {}
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

	inline RotationCoord operator+(const RotationCoord& a, const RS_FLOAT b)
	{
		return {a.azimuth + b, a.elevation + b, a.t};
	}

	inline RotationCoord operator*(const RotationCoord& a, const RS_FLOAT b)
	{
		return {a.azimuth * b, a.elevation * b, a.t};
	}

	inline RotationCoord operator/(const RS_FLOAT a, const RotationCoord& b)
	{
		return {a / b.azimuth, a / b.elevation, b.t};
	}

	inline RotationCoord operator/(const RotationCoord& b, const RS_FLOAT a)
	{
		return {b.azimuth / a, b.elevation / a, b.t};
	}
}

#endif //COORD_H
