// prototype_timing.cpp
// Created by David Young on 9/17/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "prototype_timing.h"

#include "core/logging.h"

using logging::Level;

namespace timing
{
	void PrototypeTiming::setAlpha(const RealType alpha, const RealType weight)
	{
		_alphas.emplace_back(alpha);
		_weights.emplace_back(weight);
	}

	void PrototypeTiming::copyAlphas(std::vector<RealType>& alphas, std::vector<RealType>& weights) const
	{
		alphas = _alphas;
		weights = _weights;
	}

	void PrototypeTiming::setFreqOffset(const RealType offset)
	{
		if (_random_freq) { logOffsetConflict("frequency"); }
		_freq_offset = offset;
	}

	void PrototypeTiming::setPhaseOffset(const RealType offset)
	{
		if (_random_phase) { logOffsetConflict("phase"); }
		_phase_offset = offset;
	}

	void PrototypeTiming::setRandomFreqOffset(const RealType stdev)
	{
		if (_freq_offset) { logOffsetConflict("frequency"); }
		_random_freq = stdev;
	}

	void PrototypeTiming::setRandomPhaseOffset(const RealType stdev)
	{
		if (_phase_offset) { logOffsetConflict("phase"); }
		_random_phase = stdev;
	}

	void PrototypeTiming::logOffsetConflict(const std::string& offsetType) const
	{
		LOG(Level::ERROR,
		    "Random {0} offset and constant {0} offset are set for timing source {1}. Only the random offset will be used.",
		    offsetType.c_str(), _name.c_str());
	}
}
