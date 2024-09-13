// object.h
// Base class for all physical objects in the world
// Marc Brooker
// 19 July 2006

#ifndef OBJECT_H
#define OBJECT_H

#include <string>
#include <boost/utility.hpp>

#include "config.h"
#include "radar/platform.h"

namespace rs
{
	class Platform;

	class Object : boost::noncopyable
	{
	public:
		Object(const Platform* platform, std::string name) : _platform(platform), _name(std::move(name))
		{
		}

		virtual ~Object() = default;

		[[nodiscard]] Vec3 getPosition(const RS_FLOAT time) const
		{
			return _platform->getPosition(time);
		}

		[[nodiscard]] SVec3 getRotation(const RS_FLOAT time) const
		{
			return _platform->getRotation(time);
		}

		[[nodiscard]] const Platform* getPlatform() const
		{
			return _platform;
		}

		[[nodiscard]] std::string getName() const
		{
			return _name;
		}

	private:
		const Platform* _platform;
		std::string _name;
	};
}

#endif
