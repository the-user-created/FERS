// rotation_path.cpp
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "rotation_path.h"

#include <algorithm>                  // for lower_bound
#include <cmath>                      // for fmod

#include "multipath_surface.h"        // for MultipathSurface
#include "path_utils.h"               // for PathException, finalizeCubic
#include "math/coord.h"         // for RotationCoord, operator*, opera...
#include "math/geometry_ops.h"  // for SVec3, Vec3

namespace math
{
	void RotationPath::addCoord(const RotationCoord& coord)
	{
		const auto iter = std::lower_bound(_coords.begin(), _coords.end(), coord);
		_coords.insert(iter, coord);
		_final = false; // Invalidate finalization after insertion
	}

	SVec3 RotationPath::getPosition(const RealType t) const
	{
		if (!_final) { throw PathException("Finalize not called before getPosition in RotationPath."); }
		RotationCoord coord{};

		switch (_type)
		{
		case InterpType::INTERP_STATIC: getPositionStatic(coord, _coords);
			break;
		case InterpType::INTERP_LINEAR: getPositionLinear(t, coord, _coords);
			break;
		case InterpType::INTERP_CUBIC: getPositionCubic(t, coord, _coords, _dd);
			break;
		case InterpType::INTERP_CONSTANT:
			coord.azimuth = std::fmod(t * _rate.azimuth + _start.azimuth, 2 * PI);
			coord.elevation = std::fmod(t * _rate.elevation + _start.elevation, 2 * PI);
			break;
		default: throw PathException("Unknown interpolation type.");
		}

		return {1, coord.azimuth, coord.elevation};
	}

	void RotationPath::finalize()
	{
		if (!_final)
		{
			if (_type == InterpType::INTERP_CUBIC) { finalizeCubic(_coords, _dd); }
			_final = true; // Mark as finalized
		}
	}

	void RotationPath::setInterp(const InterpType setinterp) noexcept
	{
		_type = setinterp;
		_final = false; // Requiring re-finalization
	}

	void RotationPath::setConstantRate(const RotationCoord& setstart, const RotationCoord& setrate) noexcept
	{
		_start = setstart;
		_rate = setrate;
		_type = InterpType::INTERP_CONSTANT;
		_final = true; // Pre-finalized for constant rate motion
	}

	std::unique_ptr<RotationPath> reflectPath(const RotationPath* path, const MultipathSurface* surf)
	{
		auto dual_path = std::make_unique<RotationPath>(path->getType());

		dual_path->setStart(path->getStart());
		dual_path->setRate(path->getRate());

		for (const auto& coord : path->getCoords())
		{
			RotationCoord rc;
			rc.t = coord.t;

			SVec3 sv{1, coord.azimuth, coord.elevation};
			auto v = Vec3(sv);
			v = surf->reflectPoint(v);
			const SVec3 refl(v);

			rc.azimuth = refl.azimuth;
			rc.elevation = refl.elevation;
			dual_path->addCoord(rc);
		}

		dual_path->finalize();
		return dual_path;
	}
}
