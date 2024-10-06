// pulse_factory.h
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#pragma once

#include <memory>
#include <string>

#include "config.h"

namespace signal
{
	class RadarSignal;
}

namespace serial
{
	[[nodiscard]] std::unique_ptr<signal::RadarSignal> loadPulseFromFile(
		const std::string& name, const std::string& filename, RealType power, RealType carrierFreq);
}
