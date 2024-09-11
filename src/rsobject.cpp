//rsobject.cpp
//Implementation of Object from rsobject.h
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//19 July 2006

#include "rsobject.h"

#include "rsplatform.h"

using namespace rs;

Object::Object(const Platform* platform, const std::string& name):
	_platform(platform),
	_name(name)
{
}

Object::~Object() = default;

// Get the position of the object
Vec3 Object::getPosition(const RS_FLOAT time) const
{
	return _platform->getPosition(time);
}

// Get the rotation of the object from it's platform
SVec3 Object::getRotation(const RS_FLOAT time) const
{
	return _platform->getRotation(time);
}

// Get the object's name
std::string Object::getName() const
{
	return _name;
}

/// Get a pointer to the platform this object is attached to
const Platform* Object::getPlatform() const
{
	return _platform;
}
