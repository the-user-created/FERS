// coord.h
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef COORD_H
#define COORD_H

#include "config.h"
#include "geometry_ops.h"

namespace coord
{
	struct Coord
	{
		rs::Vec3 pos;
		RS_FLOAT t;

		bool operator<(const Coord& b) const { return t < b.t; }

		Coord& operator=(const RS_FLOAT a)
		{
			t = a;
			pos.x = pos.y = pos.z = a;
			return *this;
		}
	};

	Coord operator*(const Coord& a, const Coord& b);

	Coord operator+(const Coord& a, const Coord& b);

	Coord operator-(const Coord& a, const Coord& b);

	Coord operator/(const Coord& a, const Coord& b);

	Coord operator+(const Coord& a, RS_FLOAT b);

	Coord operator*(const Coord& a, RS_FLOAT b);

	Coord operator/(RS_FLOAT a, const Coord& b);

	Coord operator/(const Coord& b, RS_FLOAT a);

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

		explicit RotationCoord(const RS_FLOAT a = 0) : azimuth(a), elevation(a), t(a) {}
	};

	RotationCoord operator*(const RotationCoord& a, const RotationCoord& b);

	RotationCoord operator+(const RotationCoord& a, const RotationCoord& b);

	RotationCoord operator-(const RotationCoord& a, const RotationCoord& b);

	RotationCoord operator/(const RotationCoord& a, const RotationCoord& b);

	RotationCoord operator+(const RotationCoord& a, RS_FLOAT b);

	RotationCoord operator*(const RotationCoord& a, RS_FLOAT b);

	RotationCoord operator/(RS_FLOAT a, const RotationCoord& b);

	RotationCoord operator/(const RotationCoord& b, RS_FLOAT a);
}

#endif //COORD_H
