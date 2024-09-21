// rotation_path.h
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef ROTATION_PATH_H
#define ROTATION_PATH_H

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

		explicit RotationPath(InterpType type = InterpType::INTERP_STATIC);

		void addCoord(const RotationCoord& coord);

		void finalize();

		[[nodiscard]] SVec3 getPosition(RS_FLOAT t) const;

		void setInterp(InterpType setinterp);

		void setConstantRate(const RotationCoord& setstart, const RotationCoord& setrate);

		[[nodiscard]] std::vector<RotationCoord> getCoords() const { return _coords; }

		[[nodiscard]] RotationCoord getStart() const { return _start; }

		[[nodiscard]] RotationCoord getRate() const { return _rate; }

		[[nodiscard]] InterpType getType() const { return _type; }

		void setStart(const RotationCoord& start) { _start = start; }

		void setRate(const RotationCoord& rate) { _rate = rate; }

	private:
		std::vector<RotationCoord> _coords;
		std::vector<RotationCoord> _dd;
		bool _final;
		RotationCoord _start;
		RotationCoord _rate;
		InterpType _type;
	};

	std::unique_ptr<RotationPath> reflectPath(const RotationPath* path, const MultipathSurface* surf);
}

#endif //ROTATION_PATH_H
