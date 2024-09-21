// prototype_timing.cpp
// Created by David Young on 9/17/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "prototype_timing.h"

#include "core/logging.h"

// =====================================================================================================================
//
// PROTOTYPE TIMING CLASS
//
// =====================================================================================================================

namespace rs
{
	void PrototypeTiming::addAlpha(const RS_FLOAT alpha, const RS_FLOAT weight)
	{
		_alphas.emplace_back(alpha);
		_weights.emplace_back(weight);
	}

	void PrototypeTiming::getAlphas(std::vector<RS_FLOAT>& getAlphas, std::vector<RS_FLOAT>& getWeights) const
	{
		getAlphas = _alphas;
		getWeights = _weights;
	}

	void PrototypeTiming::addFreqOffset(const RS_FLOAT offset)
	{
		if (_random_freq.has_value())
		{
			LOG(logging::Level::ERROR,
			    "Random frequency offset and constant frequency offset are set for timing source {}. Only the random offset will be used.",
			    getName().c_str());
		}
		_freq_offset = offset;
	}

	void PrototypeTiming::addPhaseOffset(const RS_FLOAT offset)
	{
		if (_random_phase.has_value())
		{
			LOG(logging::Level::ERROR,
			    "Random phase offset and constant phase offset are set for timing source {}. Only the random offset will be used.",
			    getName().c_str());
		}
		_phase_offset = offset;
	}

	void PrototypeTiming::addRandomFreqOffset(const RS_FLOAT stdev)
	{
		if (_freq_offset.has_value())
		{
			LOG(logging::Level::ERROR,
			    "Random frequency offset and constant frequency offset are set for timing source {}. Only the random offset will be used.",
			    getName().c_str());
		}
		_random_freq = stdev;
	}

	void PrototypeTiming::addRandomPhaseOffset(const RS_FLOAT stdev)
	{
		if (_phase_offset.has_value())
		{
			LOG(logging::Level::ERROR,
			    "Random phase offset and constant phase offset are set for timing source {}. Only the random offset will be used.",
			    getName().c_str());
		}
		_random_phase = stdev;
	}
}
