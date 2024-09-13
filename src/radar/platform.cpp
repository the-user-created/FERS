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
	if (plat->getDual()) { return plat->getDual(); }
	auto* dual = new Platform(plat->getName() + "_dual");
	const_cast<Platform*>(plat)->setDual(dual);
	dual->setMotionPath(reflectPath(plat->getMotionPath(), surf));
	dual->setRotationPath(reflectPath(plat->getRotationPath(), surf));
	return dual;
}
