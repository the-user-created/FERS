// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
* @file path.cpp
* @brief Implementation of the Path class.
*/

#include <libfers/path.h>

#include <algorithm>

#include <libfers/coord.h>
#include "path_utils.h"
#include <libfers/logging.h>
#include <libfers/geometry_ops.h>

using logging::Level;

namespace math
{
	void Path::addCoord(const Coord& coord) noexcept
	{
		auto comp = [](const Coord& a, const Coord& b) { return a.t < b.t; };

		const auto iter = std::ranges::lower_bound(_coords, coord, comp);
		_coords.insert(iter, coord);
		_final = false;
	}

	Vec3 Path::getPosition(const RealType t) const
	{
		if (!_final)
		{
			LOG(Level::FATAL, "Finalize not called before GetPosition");
			throw PathException("Finalize not called before GetPosition");
		}

		Coord coord{};
		switch (_type)
		{
		case InterpType::INTERP_STATIC:
			getPositionStatic(coord, _coords);
			break;
		case InterpType::INTERP_LINEAR:
			getPositionLinear(t, coord, _coords);
			break;
		case InterpType::INTERP_CUBIC:
			getPositionCubic(t, coord, _coords, _dd);
			break;
		}
		return coord.pos;
	}

	void Path::finalize()
	{
		if (!_final)
		{
			switch (_type)
			{
			case InterpType::INTERP_STATIC:
			case InterpType::INTERP_LINEAR:
				break;
			case InterpType::INTERP_CUBIC:
				finalizeCubic<Coord>(_coords, _dd);
				break;
			}
			_final = true;
		}
	}

	void Path::setInterp(const InterpType settype) noexcept
	{
		_type = settype;
		_final = false;
	}
}
