// platform.cpp
// Implementation of simulator platform class
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 26 April 2006

#include "platform.h"

#include "core/logging.h"

namespace radar
{
	Platform* createMultipathDual(const Platform* plat, const math::MultipathSurface* surf)
	{
		if (plat->getDual().has_value())
		{
			LOG(logging::Level::DEBUG, "[Platform.createMultipathDual] Dual platform already exists");
			return plat->getDual().value();
		}
		auto dual = std::make_unique<Platform>(plat->getName() + "_dual");
		const_cast<Platform*>(plat)->setDual(dual.get());

		dual->setMotionPath(reflectPath(plat->getMotionPath(), surf));
		dual->setRotationPath(reflectPath(plat->getRotationPath(), surf));

		return dual.release();
	}
}
