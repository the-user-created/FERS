// platform.cpp
// Implementation of simulator platform class
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 26 April 2006

#include "platform.h"

using namespace rs;

Platform::~Platform()
{
	delete _motion_path;
	delete _rotation_path;
}

Platform* rs::createMultipathDual(const Platform* plat, const MultipathSurface* surf)
{
	if (plat->_dual)
	{
		return plat->_dual;
	}
	auto* dual = new Platform(plat->getName() + "_dual");
	const_cast<Platform*>(plat)->_dual = dual;
	dual->_motion_path = reflectPath(plat->_motion_path, surf);
	dual->_rotation_path = reflectPath(plat->_rotation_path, surf);
	return dual;
}
