// platform.h
// Simulator Platform Object
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 21 April 2006

#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>
#include <boost/utility.hpp>

#include "config.h"
#include "math_utils/path.h"
#include "math_utils/rotation_path.h"

namespace rs
{
	class MultipathSurface;

	class Platform : boost::noncopyable
	{
	public:
		explicit Platform(std::string name) : _motion_path(new path::Path()), _rotation_path(new path::RotationPath()),
		                                      _name(std::move(name)), _dual(nullptr) {}

		~Platform();

		[[nodiscard]] path::Path* getMotionPath() const { return _motion_path; }

		void setMotionPath(path::Path* path) { _motion_path = path; }

		[[nodiscard]] path::RotationPath* getRotationPath() const { return _rotation_path; }

		void setRotationPath(path::RotationPath* path) { _rotation_path = path; }

		[[nodiscard]] Vec3 getPosition(const RS_FLOAT time) const { return _motion_path->getPosition(time); }

		[[nodiscard]] SVec3 getRotation(const RS_FLOAT time) const { return _rotation_path->getPosition(time); }

		[[nodiscard]] std::string getName() const { return _name; }

		[[nodiscard]] Platform* getDual() const { return _dual; }

		void setDual(Platform* dual) { _dual = dual; }

	private:
		// TODO: use unique_ptr
		path::Path* _motion_path;
		path::RotationPath* _rotation_path;
		std::string _name;
		Platform* _dual;
	};

	Platform* createMultipathDual(const Platform* plat, const MultipathSurface* surf);
}

#endif
