// path.cpp
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "path.h"
#include "coord.h"
#include "multipath_surface.h"
#include "path_utils.h"
#include "core/logging.h"
#include "python/python_extension.h"

using logging::Level;

namespace math
{
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
	Vec3 Path::getPosition(const RealType t) const
	{
		Coord coord;
		if (!_final) { throw PathException("Finalize not called before GetPosition"); }
		//Call the interpolation function relevant to the type
		switch (_type)
		{
		case InterpType::INTERP_STATIC: getPositionStatic(coord, _coords);
			break;
		case InterpType::INTERP_LINEAR: getPositionLinear(t, coord, _coords);
			break;
		case InterpType::INTERP_CUBIC: getPositionCubic(t, coord, _coords, _dd);
			break;
		case InterpType::INTERP_PYTHON: if (_pythonpath == nullptr)
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
			case InterpType::INTERP_STATIC:
			case InterpType::INTERP_LINEAR:
			case InterpType::INTERP_PYTHON: break;
			case InterpType::INTERP_CUBIC: finalizeCubic<Coord>(_coords, _dd);
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
	// Note: This function is not used in the codebase
	SVec3 compare(const RealType time, const Path& start, const Path& end)
	{
		const Vec3 difference = end.getPosition(time) - start.getPosition(time);
		const SVec3 result(difference); //Get the result in spherical co-ordinates
		return result;
	}

	/// Load a python path function
	void Path::loadPythonPath(const std::string& modname, const std::string& pathname)
	{
		_pythonpath = std::make_unique<python::PythonPath>(modname, pathname);
	}

	/// Create a new path which is a reflection of this one around the given plane
	std::unique_ptr<Path> reflectPath(const Path* path, const MultipathSurface* surf)
	{
		//Don't support multipath on python paths for now
		if (path->getType() == Path::InterpType::INTERP_PYTHON)
		{
			throw std::runtime_error("Multipath surfaces are not currently supported for Python paths");
		}
		//Create a new path object
		auto dual_path = std::make_unique<Path>(path->getType());
		//Add all the coords from the current path to the old path, reflecting about the multipath plane
		for (const auto& [pos, t] : path->getCoords())
		{
			Coord refl;
			refl.t = t;
			//Reflect the point in the plane
			refl.pos = surf->reflectPoint(pos);
			LOG(Level::DEBUG, "Reflected ({}, {}, {}) to ({}, {}, {})", pos.x, pos.y,
			    pos.z, refl.pos.x, refl.pos.y, refl.pos.z);
			dual_path->addCoord(refl);
		}
		//Finalize the new path
		dual_path->finalize();
		//Done, return the new path
		return dual_path;
	}
}
