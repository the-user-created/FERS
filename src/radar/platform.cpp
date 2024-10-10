/**
 * @file platform.cpp
 * @brief Implementation of the Platform class.
 *
 * @authors David Young, Marc Brooker
 * @date 2006-04-21
 */

#include "platform.h"

#include "core/logging.h"

namespace math
{
	class MultipathSurface;
}

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
