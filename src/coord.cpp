// coord.cpp
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "coord.h"

namespace coord
{
	// TODO: This can all be simplified
	//Componentwise multiplication of space coordinates
	Coord operator*(const Coord& a, const Coord& b)
	{
		Coord c;
		c.pos = a.pos * b.pos;
		c.t = a.t;
		return c;
	}

	//Componentwise addition of space coordinates
	Coord operator+(const Coord& a, const Coord& b)
	{
		Coord c;
		c.pos = a.pos;
		c.pos += b.pos;
		c.t = a.t;
		return c;
	}

	//Componentwise subtraction of space coordinates
	Coord operator-(const Coord& a, const Coord& b)
	{
		Coord c;
		c.pos = a.pos;
		c.pos -= b.pos;
		c.t = a.t;
		return c;
	}

	//Componentwise division of space coordinates
	Coord operator/(const Coord& a, const Coord& b)
	{
		Coord c;
		c.pos = a.pos / b.pos;
		c.t = a.t;
		return c;
	}

	//Add a constant to a PathCoord
	Coord operator+(const Coord& a, const RS_FLOAT b)
	{
		Coord c;
		c.pos += b;
		c.t = a.t;
		return c;
	}

	//Multiply by a rsFloat constant
	Coord operator*(const Coord& a, const RS_FLOAT b)
	{
		Coord c;
		c.pos = a.pos * b;
		c.t = a.t;
		return c;
	}

	Coord operator/(const RS_FLOAT a, const Coord& b)
	{
		Coord c;
		c.pos = a / b.pos;
		c.t = b.t;
		return c;
	}

	Coord operator/(const Coord& b, const RS_FLOAT a)
	{
		Coord c;
		c.pos = b.pos / a;
		c.t = b.t;
		return c;
	}

	//
	// RotationCoord Implementation
	//

	//Componentwise multiplication of space coordinates
	RotationCoord operator*(const RotationCoord& a, const RotationCoord& b)
	{
		RotationCoord c;
		c.azimuth = a.azimuth * b.azimuth;
		c.elevation = a.elevation * b.elevation;
		c.t = a.t;
		return c;
	}

	//Componentwise addition of space coordinates
	RotationCoord operator+(const RotationCoord& a, const RotationCoord& b)
	{
		RotationCoord c;
		c.azimuth = a.azimuth + b.azimuth;
		c.elevation = a.elevation + b.elevation;
		c.t = a.t;
		return c;
	}

	//Componentwise subtraction of space coordinates
	RotationCoord operator-(const RotationCoord& a, const RotationCoord& b)
	{
		RotationCoord c;
		c.azimuth = a.azimuth - b.azimuth;
		c.elevation = a.elevation - b.elevation;
		c.t = a.t;
		return c;
	}

	//Componentwise division of space coordinates
	RotationCoord operator/(const RotationCoord& a, const RotationCoord& b)
	{
		RotationCoord c;
		c.azimuth = a.azimuth / b.azimuth;
		c.elevation = a.elevation / b.elevation;
		c.t = a.t;
		return c;
	}

	//Add a constant to a PathRotationCoord
	RotationCoord operator+(const RotationCoord& a, const RS_FLOAT b)
	{
		RotationCoord c;
		c.azimuth = a.azimuth + b;
		c.elevation = a.elevation + b;
		c.t = a.t;
		return c;
	}

	//Multiply by a rsFloat constant
	RotationCoord operator*(const RotationCoord& a, const RS_FLOAT b)
	{
		RotationCoord c;
		c.azimuth = a.azimuth * b;
		c.elevation = a.elevation * b;
		c.t = a.t;
		return c;
	}

	RotationCoord operator/(const RS_FLOAT a, const RotationCoord& b)
	{
		RotationCoord c;
		c.azimuth = a / b.azimuth;
		c.elevation = a / b.elevation;
		c.t = b.t;
		return c;
	}

	RotationCoord operator/(const RotationCoord& b, const RS_FLOAT a)
	{
		RotationCoord c;
		c.azimuth = b.azimuth / a;
		c.elevation = b.elevation / a;
		c.t = b.t;
		return c;
	}
}
