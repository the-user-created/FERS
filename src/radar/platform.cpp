// platform.cpp
// Implementation of simulator platform class
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 26 April 2006

#include "platform.h"

#include "core/logging.h"

namespace radar
{
	Platform* createMultipathDual(Platform* plat, const math::MultipathSurface* surf)
	{
		if (const auto dual = plat->getDual(); dual.has_value())
		{
			LOG(logging::Level::TRACE, "Dual platform already exists. Returning existing dual platform.");
			return *dual;
		}

		auto dual = std::make_unique<Platform>(plat->getName() + "_dual");

		dual->setMotionPath(reflectPath(plat->getMotionPath(), surf));
		dual->setRotationPath(reflectPath(plat->getRotationPath(), surf));

		plat->setDual(dual.get());

		return dual.release(); // Transfer ownership of dual
	}
}
