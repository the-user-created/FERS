// interpolation_point.h
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef INTERPOLATION_POINT_H
#define INTERPOLATION_POINT_H

#include "config.h"

namespace rs
{
	struct InterpPoint
	{
		InterpPoint(const RS_FLOAT power, const RS_FLOAT start, const RS_FLOAT delay, const RS_FLOAT doppler,
		            const RS_FLOAT phase, const RS_FLOAT noiseTemperature) :
			power(power), time(start), delay(delay), doppler(doppler), phase(phase),
			noise_temperature(noiseTemperature) {}

		RS_FLOAT power;
		RS_FLOAT time;
		RS_FLOAT delay;
		RS_FLOAT doppler;
		RS_FLOAT phase;
		RS_FLOAT noise_temperature;
	};
}

#endif //INTERPOLATION_POINT_H
