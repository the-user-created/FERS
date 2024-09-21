//
// Created by David Young on 9/17/24.
//

#ifndef NOISE_UTILS_H
#define NOISE_UTILS_H

#include "config.h"

namespace noise
{
	void initializeNoise();

	void cleanUpNoise();

	RS_FLOAT wgnSample(RS_FLOAT stddev);

	// Note: This function is not used in the codebase
	RS_FLOAT uniformSample();

	RS_FLOAT noiseTemperatureToPower(RS_FLOAT temperature, RS_FLOAT bandwidth);
}

#endif //NOISE_UTILS_H
