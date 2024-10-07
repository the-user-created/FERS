// rotation_path.h
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#pragma once

#include <memory>
#include <vector>

#include "config.h"
#include "coord.h"
#include "geometry_ops.h"

namespace math
{
	class MultipathSurface;

	class RotationPath
	{
	public:
		enum class InterpType { INTERP_STATIC, INTERP_CONSTANT, INTERP_LINEAR, INTERP_CUBIC };

		explicit RotationPath(const InterpType type = InterpType::INTERP_STATIC) noexcept : _type(type) {}

		void addCoord(const RotationCoord& coord) noexcept;

		void finalize();

		[[nodiscard]] const std::vector<RotationCoord>& getCoords() const noexcept { return _coords; }
		[[nodiscard]] RotationCoord getStart() const noexcept { return _start; }
		[[nodiscard]] RotationCoord getRate() const noexcept { return _rate; }
		[[nodiscard]] InterpType getType() const noexcept { return _type; }

		[[nodiscard]] SVec3 getPosition(RealType t) const;

		void setStart(const RotationCoord& start) noexcept { _start = start; }
		void setRate(const RotationCoord& rate) noexcept { _rate = rate; }

		void setInterp(InterpType setinterp) noexcept;

		void setConstantRate(const RotationCoord& setstart, const RotationCoord& setrate) noexcept;

	private:
		std::vector<RotationCoord> _coords;
		std::vector<RotationCoord> _dd;
		bool _final{false};
		RotationCoord _start{};
		RotationCoord _rate{};
		InterpType _type{InterpType::INTERP_STATIC};
	};

	std::unique_ptr<RotationPath> reflectPath(const RotationPath* path, const MultipathSurface* surf);
}
