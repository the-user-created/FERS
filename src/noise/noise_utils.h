// noise_utils.h
// Created by David Young on 9/17/24.
//

#pragma once

#include "config.h"
#include "core/parameters.h"

namespace noise
{
	RealType wgnSample(RealType stddev) noexcept;

	inline RealType noiseTemperatureToPower(const RealType temperature, const RealType bandwidth) noexcept
	{
		return params::boltzmannK() * temperature * bandwidth;
	}
}
