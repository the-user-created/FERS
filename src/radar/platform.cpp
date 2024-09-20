// platform.cpp
// Implementation of simulator platform class
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 26 April 2006

#include "platform.h"

#include "core/logging.h"

using namespace rs;

Platform* rs::createMultipathDual(const Platform* plat, const MultipathSurface* surf)
{
	if (plat->getDual().has_value())
	{
		logging::printf(logging::RS_VERBOSE, "[Platform.createMultipathDual] Dual platform already exists\n");
		return plat->getDual().value();
	}
	auto dual = std::make_unique<Platform>(plat->getName() + "_dual");
	const_cast<Platform*>(plat)->setDual(dual.get());

	dual->setMotionPath(reflectPath(plat->getMotionPath(), surf));
	dual->setRotationPath(reflectPath(plat->getRotationPath(), surf));

	return dual.release();
}
