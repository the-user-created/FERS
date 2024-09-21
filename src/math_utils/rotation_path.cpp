// rotation_path.cpp
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "rotation_path.h"

#include <cmath>

#include "multipath_surface.h"
#include "path_utils.h"

namespace math
{
	RotationPath::RotationPath(const InterpType type):
		_final(false), _start(0), _rate(0), _type(type) {}

	void RotationPath::addCoord(const RotationCoord& coord)
	{
		//Find the position to insert the coordinate, preserving sort
		const auto iter = lower_bound(_coords.begin(), _coords.end(), coord);
		//Insert the new coordinate
		_coords.insert(iter, coord);
		//We are not finalized if we have inserted a coord
		_final = false;
	}

	// Get the position of the path object at a specified time
	SVec3 RotationPath::getPosition(const RealType t) const
	{
		RotationCoord coord;
		if (!_final) { throw PathException("Finalize not called before GetPosition in Rotation"); }
		// Call the interpolation function relevant to the type
		switch (_type)
		{
		case InterpType::INTERP_STATIC: getPositionStatic(coord, _coords);
			break;
		case InterpType::INTERP_LINEAR: getPositionLinear(t, coord, _coords);
			break;
		case InterpType::INTERP_CUBIC: getPositionCubic(t, coord, _coords, _dd);
			break;
		case InterpType::INTERP_CONSTANT: coord.t = t;
			coord.azimuth = std::fmod(t * _rate.azimuth + _start.azimuth, 2 * PI);
			coord.elevation = std::fmod(t * _rate.elevation + _start.elevation, 2 * PI);
			break;
		}
		return {1, coord.azimuth, coord.elevation};
	}

	//Finalize the path - doing some once-per-path calculations if necessary
	void RotationPath::finalize()
	{
		if (!_final)
		{
			switch (_type)
			{
			case InterpType::INTERP_STATIC:
			case InterpType::INTERP_LINEAR:
			case InterpType::INTERP_CONSTANT: break;
			case InterpType::INTERP_CUBIC: finalizeCubic<RotationCoord>(_coords, _dd);
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
		_type = InterpType::INTERP_CONSTANT;
		_final = true;
	}

	/// Create a new path which is a reflection of this one around the given plane
	std::unique_ptr<RotationPath> reflectPath(const RotationPath* path, const MultipathSurface* surf)
	{
		//Create the new RotationPath object
		auto dual_path = std::make_unique<RotationPath>(path->getType());
		//Copy constant rotation params
		dual_path->setStart(path->getStart());
		dual_path->setRate(path->getRate());
		//Copy the coords, reflecting them in the surface
		for (const auto& coord : path->getCoords())
		{
			RotationCoord rc;
			//Time copies directly
			rc.t = coord.t;
			SVec3 sv(1, coord.azimuth, coord.elevation);
			Vec3 v(sv);
			//Reflect the point in the given plane
			v = surf->reflectPoint(v);
			const SVec3 refl(v);
			rc.azimuth = refl.azimuth;
			rc.elevation = refl.elevation;
			dual_path->addCoord(rc);
		}
		//Finalize the copied path
		dual_path->finalize();
		//Done, return the created object
		return dual_path;
	}
}
