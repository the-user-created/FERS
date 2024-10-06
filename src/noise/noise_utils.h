// noise_utils.h
// Created by David Young on 9/17/24.
//

#pragma once

#include "config.h"

namespace noise
{
	RealType wgnSample(RealType stddev);

	RealType noiseTemperatureToPower(RealType temperature, RealType bandwidth);
}
