// pulse_factory.h
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef PULSE_FACTORY_H
#define PULSE_FACTORY_H

#include <memory>

#include "config.h"

namespace signal
{
	class RadarSignal;
}

namespace serial
{
	std::unique_ptr<signal::RadarSignal> loadPulseFromFile(const std::string& name, const std::string& filename,
	                                                       RS_FLOAT power, RS_FLOAT carrierFreq);
}

#endif //PULSE_FACTORY_H
