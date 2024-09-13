// path.cpp
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "path.h"

#include <stdexcept>

#include "multipath_surface.h"
#include "path_utils.h"
#include "core/logging.h"
#include "python/python_extension.h"

namespace path
{
	Path::Path(const InterpType type):
		_final(false), _type(type)
	{
		_pythonpath = nullptr; //No python path, until loaded
	}

	void Path::addCoord(const coord::Coord& coord)
	{
		//Find the position to insert the coordinate, preserving sort
		const auto iter = lower_bound(_coords.begin(), _coords.end(), coord);
		//Insert the new coordinate
		_coords.insert(iter, coord);
		//We are not finalized if we have inserted a coord
		_final = false;
	}

	//Get the position of the path object at a specified time
	rs::Vec3 Path::getPosition(const RS_FLOAT t) const
	{
		coord::Coord coord;
		if (!_final) { throw PathException("Finalize not called before GetPosition"); }
		//Call the interpolation function relevant to the type
		switch (_type)
		{
		case RS_INTERP_STATIC: getPositionStatic<coord::Coord>(coord, _coords);
			break;
		case RS_INTERP_LINEAR: getPositionLinear<coord::Coord>(t, coord, _coords);
			break;
		case RS_INTERP_CUBIC: getPositionCubic<coord::Coord>(t, coord, _coords, _dd);
			break;
		case RS_INTERP_PYTHON: if (!_pythonpath)
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
			case RS_INTERP_LINEAR:
			case RS_INTERP_PYTHON: break;
			case RS_INTERP_CUBIC: finalizeCubic<coord::Coord>(_coords, _dd);
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
	rs::SVec3 compare(const RS_FLOAT time, const Path& start, const Path& end) // TODO: unused
	{
		const rs::Vec3 difference = end.getPosition(time) - start.getPosition(time);
		const rs::SVec3 result(difference); //Get the result in spherical co-ordinates
		return result;
	}

	/// Load a python path function
	void Path::loadPythonPath(const std::string& modname, const std::string& pathname)
	{
		//If we have one already, delete it
		delete _pythonpath;

		//Load the new python path
		_pythonpath = new rs_python::PythonPath(modname, pathname);
	}

	/// Create a new path which is a reflection of this one around the given plane
	Path* reflectPath(const Path* path, const rs::MultipathSurface* surf)
	{
		//Don't support multipath on python paths for now
		if (path->getPythonPath())
		{
			throw std::runtime_error("[ERROR] Multipath surfaces are not currently supported for Python paths");
		}
		//Create a new path object
		const auto dual = new Path(path->getType());
		//Add all the coords from the current path to the old path, reflecting about the multipath plane
		for (const auto& [pos, t] : path->getCoords())
		{
			coord::Coord refl;
			refl.t = t;
			//Reflect the point in the plane
			refl.pos = surf->reflectPoint(pos);
			logging::printf(logging::RS_VERBOSE, "Reflected (%g, %g, %g) to (%g, %g, %g)\n", pos.x, pos.y,
			                pos.z, refl.pos.x, refl.pos.y, refl.pos.z);
			dual->addCoord(refl);
		}
		//Finalize the new path
		dual->finalize();
		//Done, return the new path
		return dual;
	}
}
