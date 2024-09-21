// prototype_timing.cpp
// Created by David Young on 9/17/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "prototype_timing.h"

#include "core/logging.h"

using logging::Level;

namespace timing
{
	// =================================================================================================================
	//
	// PROTOTYPE TIMING CLASS
	//
	// =================================================================================================================

	void PrototypeTiming::addAlpha(const RealType alpha, const RealType weight)
	{
		_alphas.emplace_back(alpha);
		_weights.emplace_back(weight);
	}

	void PrototypeTiming::getAlphas(std::vector<RealType>& getAlphas, std::vector<RealType>& getWeights) const
	{
		getAlphas = _alphas;
		getWeights = _weights;
	}

	void PrototypeTiming::addFreqOffset(const RealType offset)
	{
		if (_random_freq.has_value())
		{
			LOG(Level::ERROR,
			    "Random frequency offset and constant frequency offset are set for timing source {}. Only the random offset will be used.",
			    getName().c_str());
		}
		_freq_offset = offset;
	}

	void PrototypeTiming::addPhaseOffset(const RealType offset)
	{
		if (_random_phase.has_value())
		{
			LOG(Level::ERROR,
			    "Random phase offset and constant phase offset are set for timing source {}. Only the random offset will be used.",
			    getName().c_str());
		}
		_phase_offset = offset;
	}

	void PrototypeTiming::addRandomFreqOffset(const RealType stdev)
	{
		if (_freq_offset.has_value())
		{
			LOG(Level::ERROR,
			    "Random frequency offset and constant frequency offset are set for timing source {}. Only the random offset will be used.",
			    getName().c_str());
		}
		_random_freq = stdev;
	}

	void PrototypeTiming::addRandomPhaseOffset(const RealType stdev)
	{
		if (_phase_offset.has_value())
		{
			LOG(Level::ERROR,
			    "Random phase offset and constant phase offset are set for timing source {}. Only the random offset will be used.",
			    getName().c_str());
		}
		_random_phase = stdev;
	}
}
