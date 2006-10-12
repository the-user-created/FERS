//rsplatform.h
//Simulator Platform Object
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 21 April 2006

#ifndef __RSPLATFORM_H
#define __RSPLATFORM_H

#include <config.h>
#include "rspath.h"
#include <string>
#include <boost/utility.hpp>

namespace rs {  
  
  /// Platform objects are the base to which all other objects are attached.
  /// They control the position and rotation of an object at all times
class Platform: boost::noncopyable {
public:
  /// Default Constructor
  Platform(std::string name = "DefaultPlatform");

  /// Destructor
  ~Platform();

  /// Return a pointer to the motion path
  Path *GetMotionPath();

  /// Return a pointer to the rotation path
  RotationPath *GetRotationPath();
 
  /// Get the position of the platform at the specified time
  Vec3 GetPosition(rsFloat time) const;
  /// Get the rotation of the platform at the specified time
  SVec3 GetRotation(rsFloat time) const;

  /// Get the name of the platform
  std::string GetName() const;

private:
  Path *motionPath; //!< Pointer to platform's motion path
  RotationPath *rotationPath; //!< Pointer to platform's rotation path
  std::string name; //!< The name of the platform
};

}

#endif
