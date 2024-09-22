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
		RealType power;
		RealType time;
		RealType delay;
		RealType doppler;
		RealType phase;
		RealType noise_temperature;
	};
}

#endif //INTERPOLATION_POINT_H
