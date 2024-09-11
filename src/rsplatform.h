//rsplatform.h
//Simulator Platform Object
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 21 April 2006

#ifndef RS_PLATFORM_H
#define RS_PLATFORM_H

#include <string>
#include <boost/utility.hpp>

#include "config.h"
#include "rspath.h"

namespace rs
{
	//Forward definition of MultipathSurface (rsmultipath.h)
	class MultipathSurface;

	/// The Platform class controls the motion and rotation of all objects in the scene
	class Platform : boost::noncopyable
	{
	public:
		/// Default Constructor
		explicit Platform(std::string  name);

		/// Destructor
		~Platform();

		/// Return a pointer to the motion path
		[[nodiscard]] Path* getMotionPath() const;

		/// Return a pointer to the rotation path
		[[nodiscard]] RotationPath* getRotationPath() const;

		/// Get the position of the platform at the specified time
		[[nodiscard]] Vec3 getPosition(RS_FLOAT time) const;

		/// Get the rotation of the platform at the specified time
		[[nodiscard]] SVec3 getRotation(RS_FLOAT time) const;

		/// Get the name of the platform
		[[nodiscard]] std::string getName() const;

	private:
		Path* _motion_path; //!< Pointer to platform's motion path
		RotationPath* _rotation_path; //!< Pointer to platform's rotation path
		std::string _name; //!< The name of the platform
		Platform* _dual; //!< Multipath dual of this platform
		/// Create a dual of this platform for multipath simulation
		friend Platform* createMultipathDual(const Platform* plat, const MultipathSurface* surf);
	};

	/// Create a dual of this platform for multipath simulation
	Platform* createMultipathDual(const Platform* plat, const MultipathSurface* surf);
}

namespace rs
{
	// TODO: Duplicate declaration of createMultipathDual?
	Platform* createMultipathDual(const Platform* plat, const MultipathSurface* surf);
}
#endif
