/**
 * @file path.cpp
 * @brief Implementation of the Path class.
 *
 * @authors David Young, Marc Brooker
 * @date 2024-09-13
 */

#include "path.h"

#include <algorithm>
#include <stdexcept>

#include "coord.h"
#include "multipath_surface.h"
#include "path_utils.h"
#include "core/logging.h"
#include "math/geometry_ops.h"
#include "python/python_extension.h"

using logging::Level;

namespace math
{
	void Path::addCoord(const Coord& coord) noexcept
	{
		// Custom comparator to compare Coord objects based on the member pointer &Coord::t
		auto comp = [](const Coord& a, const Coord& b) { return a.t < b.t; };

		// Insert the new coordinate while preserving sort order using std::ranges
		const auto iter = std::ranges::lower_bound(_coords, coord, comp);
		_coords.insert(iter, coord);
		_final = false; // Invalidate the finalization after inserting a new coordinate
	}

	Vec3 Path::getPosition(const RealType t) const
	{
		if (!_final)
		{
			LOG(Level::FATAL, "Finalize not called before GetPosition");
			throw PathException("Finalize not called before GetPosition");
		}

		Coord coord{};
		// Call the appropriate interpolation function based on _type
		switch (_type)
		{
		case InterpType::INTERP_STATIC: getPositionStatic(coord, _coords);
			break;
		case InterpType::INTERP_LINEAR: getPositionLinear(t, coord, _coords);
			break;
		case InterpType::INTERP_CUBIC: getPositionCubic(t, coord, _coords, _dd);
			break;
		case InterpType::INTERP_PYTHON: if (!_pythonpath)
			{
				LOG(Level::FATAL, "Python path GetPosition called before module loaded");
				throw std::logic_error("Python path GetPosition called before module loaded");
			}
			return _pythonpath->getPosition(t);
		}
		return coord.pos; // Return the position part of the coordinate
	}

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
			_final = true; // Mark as finalized
		}
	}

	void Path::setInterp(const InterpType settype) noexcept
	{
		_type = settype;
		_final = false; // Invalidate the finalization after changing interpolation type
	}

	void Path::setPythonPath(const std::string_view modname, const std::string_view pathname)
	{
		_pythonpath = std::make_unique<python::PythonPath>(modname.data(), pathname.data());
	}

	std::unique_ptr<Path> reflectPath(const Path* path, const MultipathSurface* surf)
	{
		if (path->getType() == Path::InterpType::INTERP_PYTHON)
		{
			LOG(Level::FATAL, "Multipath surfaces are not currently supported for Python paths");
			throw std::runtime_error("Multipath surfaces are not currently supported for Python paths");
		}

		// Create a new path object
		auto dual_path = std::make_unique<Path>(path->getType());

		// Reflect all coordinates and add them to the new path
		for (const auto& [pos, t] : path->getCoords())
		{
			Coord refl{};
			refl.t = t;
			refl.pos = surf->reflectPoint(pos); // Reflect the point in the plane

			LOG(Level::DEBUG, "Reflected ({}, {}, {}) to ({}, {}, {})",
			    pos.x, pos.y, pos.z,
			    refl.pos.x, refl.pos.y, refl.pos.z);

			dual_path->addCoord(refl);
		}

		dual_path->finalize(); // Finalize the new path
		return dual_path; // Return the new path
	}
}
