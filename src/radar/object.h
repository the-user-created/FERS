// object.h
// Base class for all physical objects in the world
// Marc Brooker
// 19 July 2006

#ifndef OBJECT_H
#define OBJECT_H

#include "platform.h"

namespace radar
{
	class Object
	{
	public:
		Object(const Platform* platform, std::string name) : _platform(platform), _name(std::move(name)) {}

		virtual ~Object() = default;

		// Delete the copy constructor and copy assignment operator
		Object(const Object&) = delete;

		Object& operator=(const Object&) = delete;

		[[nodiscard]] math::Vec3 getPosition(const RS_FLOAT time) const { return _platform->getPosition(time); }

		[[nodiscard]] math::SVec3 getRotation(const RS_FLOAT time) const { return _platform->getRotation(time); }

		[[nodiscard]] const Platform* getPlatform() const { return _platform; }

		[[nodiscard]] std::string getName() const { return _name; }

	private:
		const Platform* _platform;
		std::string _name;
	};
}

#endif
