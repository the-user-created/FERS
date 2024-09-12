// platform.h
// Simulator Platform Object
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 21 April 2006

#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>
#include <boost/utility.hpp>

#include "config.h"
#include "rspath.h"

namespace rs
{
	class MultipathSurface;

	class Platform : boost::noncopyable
	{
	public:
		explicit Platform(std::string name) : _motion_path(new Path()), _rotation_path(new RotationPath()), _name(std::move(name)), _dual(nullptr) {}

		~Platform();

		[[nodiscard]] Path* getMotionPath() const { return _motion_path; }

		[[nodiscard]] RotationPath* getRotationPath() const { return _rotation_path; }

		[[nodiscard]] Vec3 getPosition(const RS_FLOAT time) const { return _motion_path->getPosition(time); }

		[[nodiscard]] SVec3 getRotation(const RS_FLOAT time) const { return _rotation_path->getPosition(time); }

		[[nodiscard]] std::string getName() const { return _name; }

	private:
		// TODO: use unique_ptr
		Path* _motion_path;
		RotationPath* _rotation_path;
		std::string _name;
		Platform* _dual;

		friend Platform* createMultipathDual(const Platform* plat, const MultipathSurface* surf);
	};

	Platform* createMultipathDual(const Platform* plat, const MultipathSurface* surf);
}

#endif
