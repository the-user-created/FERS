//rsplatform.cpp
//Implementation of simulator platform class
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 26 April 2006

#include "rsplatform.h"

using namespace rs;

//Default constructor
Platform::Platform(std::string name):
  name(name)
{
  motionPath = new Path();
  rotationPath = new RotationPath();
}

//Default destructor
Platform::~Platform()
{
  delete motionPath;
  delete rotationPath;
}

//Get the current position of the platform
Vec3 Platform::GetPosition(rsFloat time) const
{
  return motionPath->GetPosition(time);
}

//Get the current rotation of the platform
SVec3 Platform::GetRotation(rsFloat time) const
{
  return rotationPath->GetPosition(time);
}

//Return a pointer to the motion path
Path* Platform::GetMotionPath()
{
  return motionPath;
}

//Return a pointer to the rotation path
RotationPath* Platform::GetRotationPath()
{
  return rotationPath;
}

//Return the name of the platform
std::string Platform::GetName() const
{
  return name;
}
