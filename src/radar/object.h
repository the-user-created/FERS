// object.h
// Base class for all physical objects in the world
// Marc Brooker
// 19 July 2006

#pragma once

#include "platform.h"

namespace radar
{
	class Object
	{
	public:
		Object(Platform* platform, std::string name) : _platform(platform), _name(std::move(name)) {}

		// Default virtual destructor
		virtual ~Object() = default;

		// Delete copy constructor and copy assignment operator
		Object(const Object&) = delete;

		Object& operator=(const Object&) = delete;

		// Default move constructor and move assignment operator
		Object(Object&&) noexcept = default;

		Object& operator=(Object&&) noexcept = default;

		[[nodiscard]] math::Vec3 getPosition(const RealType time) const { return _platform->getPosition(time); }
		[[nodiscard]] math::SVec3 getRotation(const RealType time) const { return _platform->getRotation(time); }
		[[nodiscard]] Platform* getPlatform() const { return _platform; }
		[[nodiscard]] const std::string& getName() const { return _name; }

	private:
		Platform* _platform;
		std::string _name;
	};
}
