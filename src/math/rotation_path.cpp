/**
 * @file rotation_path.cpp
 * @brief Implementation of the RotationPath class.
 *
 * @authors David Young, Marc Brooker
 * @date 2024-09-13
 */

#include "rotation_path.h"

#include <algorithm>
#include <cmath>

#include "multipath_surface.h"
#include "path_utils.h"
#include "math/coord.h"
#include "math/geometry_ops.h"

namespace math
{
	void RotationPath::addCoord(const RotationCoord& coord) noexcept
	{
		const auto iter = std::lower_bound(_coords.begin(), _coords.end(), coord);
		_coords.insert(iter, coord);
		_final = false;
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
			_final = true;
		}
	}

	void RotationPath::setInterp(const InterpType setinterp) noexcept
	{
		_type = setinterp;
		_final = false;
	}

	void RotationPath::setConstantRate(const RotationCoord& setstart, const RotationCoord& setrate) noexcept
	{
		_start = setstart;
		_rate = setrate;
		_type = InterpType::INTERP_CONSTANT;
		_final = true;
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
