//rsobject.h
//Base class for all physical objects in the world
//Marc Brooker
//19 July 2006

#ifndef RS_OBJECT_H
#define RS_OBJECT_H

#include <config.h>
#include <string>
#include <boost/utility.hpp>

#include "rspath.h"

namespace rs
{
	// Forward definition of Platform (see rsplatform.h)
	class Platform;

	/// Object is a physical object in the world, it embodies the properties common to all physical objects
	class Object : boost::noncopyable
	{
	public:
		Object(const Platform* platform, const std::string& name);

		~Object();

		/// Get the position of the object in space
		Vec3 getPosition(rsFloat time) const;

		/// Get the rotation of the object in space
		SVec3 getRotation(rsFloat time) const;

		/// Get a pointer to the platform this object is attached to
		const Platform* getPlatform() const;

		/// Get the name of the object
		std::string getName() const;

	private:
		const Platform* _platform; //<! Platform to which the object is attached
		std::string _name; //<! Name of the object
	};
}

#endif
