//rsplatform.cpp
//Implementation of simulator platform class
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 26 April 2006

#include "rsplatform.h"

#include "rsmultipath.h"

using namespace rs;

//Default constructor
Platform::Platform(const std::string& name):
	_name(name),
	_dual(nullptr)
{
	_motion_path = new Path();
	_rotation_path = new RotationPath();
}

//Default destructor
Platform::~Platform()
{
	delete _motion_path;
	delete _rotation_path;
}

//Get the current position of the platform
Vec3 Platform::getPosition(const RS_FLOAT time) const
{
	return _motion_path->getPosition(time);
}

//Get the current rotation of the platform
SVec3 Platform::getRotation(const RS_FLOAT time) const
{
	return _rotation_path->getPosition(time);
}

//Return a pointer to the motion path
Path* Platform::getMotionPath() const
{
	return _motion_path;
}

//Return a pointer to the rotation path
RotationPath* Platform::getRotationPath() const
{
	return _rotation_path;
}

//Return the name of the platform
std::string Platform::getName() const
{
	return _name;
}

/// Create a dual of this platform for multipath simulation
Platform* rs::createMultipathDual(const Platform* plat, const MultipathSurface* surf)
{
	//If the dual already exists, just return it
	if (plat->_dual)
	{
		return plat->_dual;
	}
	//Create the new platform
	Platform* dual = new Platform(plat->getName() + "_dual");
	//Set the platform dual
	(const_cast<Platform*>(plat))->_dual = dual;
	//Reflect the paths used to guide the platform
	dual->_motion_path = reflectPath(plat->_motion_path, surf);
	dual->_rotation_path = reflectPath(plat->_rotation_path, surf);
	//Done, return the created object
	return dual;
}
