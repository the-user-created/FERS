// interpolation_point.h
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef INTERPOLATION_POINT_H
#define INTERPOLATION_POINT_H

namespace interp
{
	struct InterpPoint
	{
		InterpPoint(const RealType power, const RealType start, const RealType delay, const RealType doppler,
		            const RealType phase, const RealType noiseTemperature) :
			power(power), time(start), delay(delay), doppler(doppler), phase(phase),
			noise_temperature(noiseTemperature) {}

		RealType power;
		RealType time;
		RealType delay;
		RealType doppler;
		RealType phase;
		RealType noise_temperature;
	};
}

#endif //INTERPOLATION_POINT_H
