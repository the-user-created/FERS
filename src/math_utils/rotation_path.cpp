// rotation_path.cpp
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "rotation_path.h"

#include "multipath_surface.h"
#include "path_utils.h"

namespace path
{
	RotationPath::RotationPath(const InterpType type):
		_final(false), _start(0), _rate(0), _type(type) {}

	void RotationPath::addCoord(const coord::RotationCoord& coord)
	{
		//Find the position to insert the coordinate, preserving sort
		const auto iter = lower_bound(_coords.begin(), _coords.end(), coord);
		//Insert the new coordinate
		_coords.insert(iter, coord);
		//We are not finalized if we have inserted a coord
		_final = false;
	}

	// Get the position of the path object at a specified time
	rs::SVec3 RotationPath::getPosition(const RS_FLOAT t) const
	{
		coord::RotationCoord coord;
		if (!_final) { throw PathException("Finalize not called before GetPosition in Rotation"); }
		// Call the interpolation function relevant to the type
		switch (_type)
		{
		case RS_INTERP_STATIC: getPositionStatic(coord, _coords);
			break;
		case RS_INTERP_LINEAR: getPositionLinear(t, coord, _coords);
			break;
		case RS_INTERP_CUBIC: getPositionCubic(t, coord, _coords, _dd);
			break;
		case RS_INTERP_CONSTANT: coord.t = t;
			coord.azimuth = std::fmod(t * _rate.azimuth + _start.azimuth, 2 * M_PI);
			coord.elevation = std::fmod(t * _rate.elevation + _start.elevation, 2 * M_PI);
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
			case RS_INTERP_STATIC:
			case RS_INTERP_LINEAR:
			case RS_INTERP_CONSTANT: break;
			case RS_INTERP_CUBIC: finalizeCubic<coord::RotationCoord>(_coords, _dd);
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
	void RotationPath::setConstantRate(const coord::RotationCoord& setstart, const coord::RotationCoord& setrate)
	{
		_start = setstart;
		_rate = setrate;
		_type = RS_INTERP_CONSTANT;
		_final = true;
	}

	/// Create a new path which is a reflection of this one around the given plane
	std::unique_ptr<RotationPath> reflectPath(const RotationPath* path, const rs::MultipathSurface* surf)
	{
		//Create the new RotationPath object
		auto dual_path = std::make_unique<RotationPath>(path->getType());
		//Copy constant rotation params
		dual_path->setStart(path->getStart());
		dual_path->setRate(path->getRate());
		//Copy the coords, reflecting them in the surface
		for (const auto& coord : path->getCoords())
		{
			coord::RotationCoord rc;
			//Time copies directly
			rc.t = coord.t;
			rs::SVec3 sv(1, coord.azimuth, coord.elevation);
			rs::Vec3 v(sv);
			//Reflect the point in the given plane
			v = surf->reflectPoint(v);
			const rs::SVec3 refl(v);
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
