//rspath.cpp
//Implementation of rotation and position path classes
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//21 April 2006

#include "rspath.h"

#include <algorithm>
#include <cmath> //for fmod

#include "rsdebug.h"
#include "rsmultipath.h"
#include "rspython.h"

using namespace rs;

//The interpolation functions are implemented as template functions to ease adding more functions to both rotation and motion classes

//Static "interpolation" function - the path is at the same point all the time
template <typename T>
void getPositionStatic(T& coord, const std::vector<T>& coords)
{
	if (coords.empty())
	{
		throw PathException("coord list empty during GetPositionStatic");
	}
	coord = coords[0];
}

//Linear interpolation function
template <typename T>
void getPositionLinear(RS_FLOAT t, T& coord, const std::vector<T>& coords)
{
	T s_key;
	s_key = 0;
	s_key.t = t;
	typename std::vector<T>::const_iterator xrp;
	xrp = upper_bound(coords.begin(), coords.end(), s_key);
	//Check if we are over one of the end points
	if (xrp == coords.begin())
	{
		coord = *xrp; //We are at the left endpoint
	}
	else if (xrp == coords.end())
	{
		coord = *(xrp - 1); //We are at the right endpoint
	}
	else
	{
		//We are at neither endpoint - perform linear interpolation
		int xri = xrp - coords.begin();
		int xli = xri - 1;

		RS_FLOAT iw = coords[xri].t - coords[xli].t;
		RS_FLOAT rw = (coords[xri].t - t) / iw;
		RS_FLOAT lw = 1 - rw;
		//Insert the interpolated values in coord
		coord = coords[xri] * lw + coords[xli] * rw;
	}
	//Set the time part of the coordinate
	coord.t = t;
}

//Cubic spline interpolation function
//The method used (but not the code) is from
//Numerical Recipes in C, Second Edition by Press, et al. pages 114-116
template <typename T>
void getPositionCubic(RS_FLOAT t, T& coord, const std::vector<T>& coords, const std::vector<T>& dd)
{
	T s_key;
	s_key = 0;
	s_key.t = t;
	typename std::vector<T>::const_iterator xrp;
	//Check that we are finalized, if not, complain
	xrp = upper_bound(coords.begin(), coords.end(), s_key);
	//Check if we are over one of the end points
	if (xrp == coords.begin())
	{
		coord = *xrp; //We are at the left endpoint
	}
	else if (xrp == coords.end())
	{
		coord = *(xrp - 1); //We are at the right endpoint
	}
	else
	{
		//We are at neither endpoint - perform cubic spline interpolation
		int xri = xrp - coords.begin();
		int xli = xri - 1;
		const RS_FLOAT xrd = coords[xri].t - t, xld = t - coords[xli].t, iw = coords[xri].t - coords[xli].t, iws = iw *
			        iw / 6.0;
		RS_FLOAT a = xrd / iw, b = xld / iw, c = (a * a * a - a) * iws, d = (b * b * b - b) * iws;
		coord = coords[xli] * a + coords[xri] * b + dd[xli] * c + dd[xri] * d;
	}
	//Set the time part of the coordinate
	coord.t = t;
}

//Finalize function to calculate vector of second derivatives
//The method used (but not the code) is from
//Numerical Recipes in C, Second Edition by Press, et al. pages 114-116
template <typename T>
void finalizeCubic(std::vector<T>& coords, std::vector<T>& dd)
{
	int size = coords.size();
	std::vector<T> tmp(size);
	dd.resize(size);

	//Set the second derivative at the end points to zero
	dd[0] = 0;
	dd[size - 1] = 0;
	//Forward pass of calculating the second derivatives at each point
	for (int i = 1; i < size - 1; i++)
	{
		T yrd = coords[i + 1] - coords[i], yld = coords[i] - coords[i - 1];
		RS_FLOAT xrd = coords[i + 1].t - coords[i].t, xld = coords[i].t - coords[i - 1].t;
		RS_FLOAT iw = coords[i + 1].t - coords[i - 1].t;
		RS_FLOAT si = xld / iw;
		T p = dd[i - 1] * si + 2.0;
		dd[i] = (si - 1.0) / p;
		tmp[i] = ((yrd / xrd - yld / xld) * 6.0 / iw - tmp[i - 1] * si) / p;
	}
	//Second (backward) pass of calculation
	for (int i = size - 2; i >= 0; --i)
	{
		dd[i] = dd[i] * dd[i + 1] + tmp[i];
	}
}


//
// Path Implementation
//
Path::Path(const InterpType type):
	_final(false), _type(type)
{
	_pythonpath = nullptr; //No python path, until loaded
}

void Path::addCoord(const Coord& coord)
{
	//Find the position to insert the coordinate, preserving sort
	const auto iter = lower_bound(_coords.begin(), _coords.end(), coord);
	//Insert the new coordinate
	_coords.insert(iter, coord);
	//We are not finalized if we have inserted a coord
	_final = false;
}

//Get the position of the path object at a specified time
Vec3 Path::getPosition(const RS_FLOAT t) const
{
	Coord coord;
	if (!_final)
	{
		throw PathException("Finalize not called before GetPosition");
	}
	//Call the interpolation function relevent to the type
	switch (_type)
	{
	case RS_INTERP_STATIC:
		getPositionStatic<Coord>(coord, _coords);
		break;
	case RS_INTERP_LINEAR:
		getPositionLinear<Coord>(t, coord, _coords);
		break;
	case RS_INTERP_CUBIC:
		getPositionCubic<Coord>(t, coord, _coords, _dd);
		break;
	case RS_INTERP_PYTHON:
		if (!_pythonpath)
		{
			throw std::logic_error("Python path GetPosition called before module loaded");
		}
		return _pythonpath->getPosition(t);
	}
	//Return the position part of the result
	return coord.pos;
}

//Finalize the path - doing some once-per-path calculations if necessary
void Path::finalize()
{
	if (!_final)
	{
		switch (_type)
		{
		case RS_INTERP_STATIC:
			break;
		case RS_INTERP_LINEAR:
			break;
		case RS_INTERP_CUBIC:
			finalizeCubic<Coord>(_coords, _dd);
			break;
		case RS_INTERP_PYTHON:
			break;
		}
		_final = true;
	}
}

//Set the interpolation type of the path
void Path::setInterp(const InterpType settype)
{
	_final = false;
	_type = settype;
}

//Compares two paths at the same time and returns a vector with the distance and angle
SVec3 compare(const RS_FLOAT time, const Path& start, const Path& end)
{
	const Vec3 difference = end.getPosition(time) - start.getPosition(time);
	SVec3 result(difference); //Get the result in spherical co-ordinates
	return result;
}

/// Load a python path function
void Path::loadPythonPath(const std::string& modname, const std::string& pathname)
{
	//If we have one already, delete it
	if (_pythonpath)
	{
		delete _pythonpath;
	}
	//Load the new python path
	_pythonpath = new rs_python::PythonPath(modname, pathname);
}

/// Create a new path which is a reflection of this one around the given plane
Path* rs::reflectPath(const Path* path, const MultipathSurface* surf)
{
	//Don't support multipath on python paths for now
	if (path->_pythonpath)
	{
		throw std::runtime_error("[ERROR] Multipath surfaces are not currently supported for Python paths");
	}
	//Create a new path object
	const auto dual = new Path(path->_type);
	//Add all the coords from the current path to the old path, reflecting about the multipath plane
	for (auto iter = path->_coords.begin(); iter != path->_coords.end(); ++iter)
	{
		Coord refl;
		refl.t = iter->t;
		//Reflect the point in the plane
		refl.pos = surf->reflectPoint(iter->pos);
		rs_debug::printf(rs_debug::RS_VERBOSE, "Reflected (%g, %g, %g) to (%g, %g, %g)\n", iter->pos.x, iter->pos.y,
		                iter->pos.z, refl.pos.x, refl.pos.y, refl.pos.z);
		dual->addCoord(refl);
	}
	//Finalize the new path
	dual->finalize();
	//Done, return the new path
	return dual;
}

//
// RotationPath Implementation
//
RotationPath::RotationPath(const InterpType type):
	_final(false), _start(0), _rate(0), _type(type)
{
}

void RotationPath::addCoord(const RotationCoord& coord)
{
	//Find the position to insert the coordinate, preserving sort
	const auto iter = lower_bound(_coords.begin(), _coords.end(), coord);
	//Insert the new coordinate
	_coords.insert(iter, coord);
	//We are not finalized if we have inserted a coord
	_final = false;
}

//Get the position of the path object at a specified time
SVec3 RotationPath::getPosition(const RS_FLOAT t) const
{
	RotationCoord coord;
	if (!_final)
	{
		throw PathException("Finalize not called before GetPosition in Rotation");
	}
	//Call the interpolation function relevent to the type
	switch (_type)
	{
	case RS_INTERP_STATIC:
		getPositionStatic<RotationCoord>(coord, _coords);
		break;
	case RS_INTERP_LINEAR:
		getPositionLinear<RotationCoord>(t, coord, _coords);
		break;
	case RS_INTERP_CUBIC:
		getPositionCubic<RotationCoord>(t, coord, _coords, _dd);
		break;
	case RS_INTERP_CONSTANT:
		coord.t = t;
		coord.azimuth = std::fmod(t * _rate.azimuth + _start.azimuth, 2 * M_PI);
		coord.elevation = std::fmod(t * _rate.elevation + _start.elevation, 2 * M_PI);
		break;
	}
	return SVec3(1, coord.azimuth, coord.elevation);
}

//Finalize the path - doing some once-per-path calculations if necessary
void RotationPath::finalize()
{
	if (!_final)
	{
		switch (_type)
		{
		case RS_INTERP_STATIC:
			break;
		case RS_INTERP_LINEAR:
			break;
		case RS_INTERP_CONSTANT:
			break;
		case RS_INTERP_CUBIC:
			finalizeCubic<RotationCoord>(_coords, _dd);
			break;
		}
		_final = true;
	}
}

//Set the interpolation type
void RotationPath::setInterp(const InterpType setinterp)
{
	_type = setinterp;
	_final = false;
}

//Set properties for fixed rate motion
void RotationPath::setConstantRate(const RotationCoord& setstart, const RotationCoord& setrate)
{
	_start = setstart;
	_rate = setrate;
	_type = RS_INTERP_CONSTANT;
	_final = true;
}

//
// Coord Implementation
//

//Componentwise multiplication of space coordinates
Coord rs::operator*(const Coord& a, const Coord& b)
{
	Coord c;
	c.pos = a.pos * b.pos;
	c.t = a.t; //Only multiply space coordinates
	return c;
}

//Componentwise addition of space coordinates
Coord rs::operator+(const Coord& a, const Coord& b)
{
	Coord c;
	c.pos = a.pos;
	c.pos += b.pos;
	c.t = a.t; //Only add space coordinates
	return c;
}

//Componentwise subtraction of space coordinates
Coord rs::operator-(const Coord& a, const Coord& b)
{
	Coord c;
	c.pos = a.pos;
	c.pos -= b.pos;
	c.t = a.t;
	return c;
}

//Componentwise division of space coordinates
Coord rs::operator/(const Coord& a, const Coord& b)
{
	Coord c;
	c.pos = a.pos / b.pos;
	c.t = a.t; //Only add space coordinates
	return c;
}

//Add a constant to a PathCoord
Coord rs::operator+(const Coord& a, const RS_FLOAT b)
{
	Coord c;
	c.pos += b;
	c.t = a.t;
	return c;
}

//Multiply by a rsFloat constant
Coord rs::operator*(const Coord& a, const RS_FLOAT b)
{
	Coord c;
	c.pos = a.pos * b;
	c.t = a.t;
	return c;
}

Coord rs::operator/(const RS_FLOAT a, const Coord& b)
{
	Coord c;
	c.pos = a / b.pos;
	c.t = b.t;
	return c;
}

Coord rs::operator/(const Coord& b, const RS_FLOAT a)
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
RotationCoord rs::operator*(const RotationCoord& a, const RotationCoord& b)
{
	RotationCoord c;
	c.azimuth = a.azimuth * b.azimuth;
	c.elevation = a.elevation * b.elevation;
	c.t = a.t; //Onlelevation multiply space coordinates
	return c;
}

//Componentwise addition of space coordinates
RotationCoord rs::operator+(const RotationCoord& a, const RotationCoord& b)
{
	RotationCoord c;
	c.azimuth = a.azimuth + b.azimuth;
	c.elevation = a.elevation + b.elevation;
	c.t = a.t; //Only add space coordinates
	return c;
}

//Componentwise subtraction of space coordinates
RotationCoord rs::operator-(const RotationCoord& a, const RotationCoord& b)
{
	RotationCoord c;
	c.azimuth = a.azimuth - b.azimuth;
	c.elevation = a.elevation - b.elevation;
	c.t = a.t;
	return c;
}

//Componentwise division of space coordinates
RotationCoord rs::operator/(const RotationCoord& a, const RotationCoord& b)
{
	RotationCoord c;
	c.azimuth = a.azimuth / b.azimuth;
	c.elevation = a.elevation / b.elevation;
	c.t = a.t; //Only add space coordinates
	return c;
}

//Add a constant to a PathRotationCoord
RotationCoord rs::operator+(const RotationCoord& a, const RS_FLOAT b)
{
	RotationCoord c;
	c.azimuth = a.azimuth + b;
	c.elevation = a.elevation + b;
	c.t = a.t;
	return c;
}

//Multiply by a rsFloat constant
RotationCoord rs::operator*(const RotationCoord& a, const RS_FLOAT b)
{
	RotationCoord c;
	c.azimuth = a.azimuth * b;
	c.elevation = a.elevation * b;
	c.t = a.t;
	return c;
}

RotationCoord rs::operator/(const RS_FLOAT a, const RotationCoord& b)
{
	RotationCoord c;
	c.azimuth = a / b.azimuth;
	c.elevation = a / b.elevation;
	c.t = b.t;
	return c;
}

RotationCoord rs::operator/(const RotationCoord& b, const RS_FLOAT a)
{
	RotationCoord c;
	c.azimuth = b.azimuth / a;
	c.elevation = b.elevation / a;
	c.t = b.t;
	return c;
}

/// Create a new path which is a reflection of this one around the given plane
RotationPath* rs::reflectPath(const RotationPath* path, const MultipathSurface* surf)
{
	//Create the new RotationPath object
	auto* dual = new RotationPath(path->_type);
	//Copy constant rotation params
	dual->_start = path->_start;
	dual->_rate = path->_rate;
	//Copy the coords, reflecting them in the surface
	for (auto iter = path->_coords.begin(); iter != path->_coords.end(); ++iter)
	{
		RotationCoord rc;
		//Time copies directly
		rc.t = iter->t;
		SVec3 sv(1, iter->azimuth, iter->elevation);
		Vec3 v(sv);
		//Reflect the point in the given plane
		v = surf->reflectPoint(v);
		const SVec3 refl(v);
		rc.azimuth = refl.azimuth;
		rc.elevation = refl.elevation;
		dual->addCoord(rc);
	}
	//Finalize the copied path
	dual->finalize();
	//Done, return the created object
	return dual;
}
