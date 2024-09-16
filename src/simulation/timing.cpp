// timing.cpp
// Implementation of timing sources
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 16 October 2006

#include "timing.h"

#include "noise_generators.h"
#include "core/logging.h"

using namespace rs;

// =====================================================================================================================
//
// PROTOTYPE TIMING CLASS
//
// =====================================================================================================================

void PrototypeTiming::addAlpha(const RS_FLOAT alpha, const RS_FLOAT weight)
{
	_alphas.push_back(alpha);
	_weights.push_back(weight);
}

void PrototypeTiming::getAlphas(std::vector<RS_FLOAT>& getAlphas, std::vector<RS_FLOAT>& getWeights) const
{
	getAlphas = _alphas;
	getWeights = _weights;
}

void PrototypeTiming::addFreqOffset(const RS_FLOAT offset)
{
	if (_random_freq != 0.0)
	{
		logging::printf(logging::RS_IMPORTANT,
		                "[Important] Random frequency offset and constant frequency offset are set for timing source %s. Only the random offset will be used.",
		                getName().c_str());
	}
	_freq_offset = offset;
}

void PrototypeTiming::addPhaseOffset(const RS_FLOAT offset)
{
	if (_random_phase != 0.0)
	{
		logging::printf(logging::RS_IMPORTANT,
		                "[Important] Random phase offset and constant phase offset are set for timing source %s. Only the random offset will be used.",
		                getName().c_str());
	}
	_phase_offset = offset;
}

void PrototypeTiming::addRandomFreqOffset(const RS_FLOAT stdev)
{
	if (_freq_offset != 0.0)
	{
		logging::printf(logging::RS_IMPORTANT,
		                "[Important] Random frequency offset and constant frequency offset are set for timing source %s. Only the random offset will be used.",
		                getName().c_str());
	}
	_random_freq = stdev;
}

void PrototypeTiming::addRandomPhaseOffset(const RS_FLOAT stdev)
{
	if (_phase_offset != 0.0)
	{
		logging::printf(logging::RS_IMPORTANT,
		                "[Important] Random phase offset and constant phase offset are set for timing source %s. Only the random offset will be used.",
		                getName().c_str());
	}
	_random_phase = stdev;
}

// =====================================================================================================================
//
// CLOCK MODEL TIMING CLASS
//
// =====================================================================================================================

void ClockModelTiming::initializeModel(const PrototypeTiming* timing)
{
	if (!_alphas.empty())
	{
		throw std::logic_error("[BUG] ClockModelTiming::InitializeModel called more than once");
	}
	timing->getAlphas(_alphas, _weights);
	_model = new ClockModelGenerator(_alphas, _weights, timing->getFrequency(), timing->getPhaseOffset(),
	                                 timing->getFreqOffset(), 15);
	if (timing->getFrequency() == 0.0)
	{
		logging::printf(logging::RS_IMPORTANT,
		                "[Important] Timing source frequency not set, results could be incorrect.");
	}
	_frequency = timing->getFrequency();
	_sync_on_pulse = timing->getSyncOnPulse();
	_enabled = true;
}

void ClockModelTiming::skipSamples(const long long samples)
{
	if (_enabled)
	{
		_model->skipSamples(samples);
	}
}
