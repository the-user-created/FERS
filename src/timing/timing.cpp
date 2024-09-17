// timing.cpp
// Implementation of timing sources
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 16 October 2006

#include "timing.h"

#include "prototype_timing.h"
#include "core/logging.h"
#include "noise/noise_generators.h"

using namespace rs;

// =====================================================================================================================
//
// CLOCK MODEL TIMING CLASS
//
// =====================================================================================================================

void ClockModelTiming::initializeModel(const PrototypeTiming* timing)
{
	if (!_alphas.empty()) { throw std::logic_error("[BUG] ClockModelTiming::initializeModel called more than once"); }

	timing->getAlphas(_alphas, _weights);

	_model = std::make_unique<ClockModelGenerator>(_alphas, _weights, timing->getFrequency(),
	                                               timing->getPhaseOffset(), timing->getFreqOffset(), 15);

	if (timing->getFrequency() == 0.0f)
	{
		logging::printf(logging::RS_IMPORTANT,
		                "[Important] Timing source frequency not set, results could be incorrect.");
	}

	_frequency = timing->getFrequency();
	_sync_on_pulse = timing->getSyncOnPulse();
	_enabled = true;
}
