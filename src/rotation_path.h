// rotation_path.h
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef ROTATION_PATH_H
#define ROTATION_PATH_H

#include <vector>

#include "coord.h"

namespace rs
{
	class MultipathSurface;
}

namespace path
{
	class RotationPath
	{
	public:
		enum InterpType { RS_INTERP_STATIC, RS_INTERP_CONSTANT, RS_INTERP_LINEAR, RS_INTERP_CUBIC };

		explicit RotationPath(InterpType type = RS_INTERP_STATIC);

		void addCoord(const coord::RotationCoord& coord);

		void finalize();

		[[nodiscard]] rs::SVec3 getPosition(RS_FLOAT t) const;

		void setInterp(InterpType setinterp);

		void setConstantRate(const coord::RotationCoord& setstart, const coord::RotationCoord& setrate);

	protected:
		std::vector<coord::RotationCoord> _coords;
		std::vector<coord::RotationCoord> _dd;
		bool _final;
		coord::RotationCoord _start;
		coord::RotationCoord _rate;
		InterpType _type;

		// TODO: Make this not a friend function and use getters
		friend RotationPath* reflectPath(const RotationPath* path, const rs::MultipathSurface* surf);
	};

	RotationPath* reflectPath(const RotationPath* path, const rs::MultipathSurface* surf);
}

#endif //ROTATION_PATH_H
