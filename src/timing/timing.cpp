// timing.cpp
// Implementation of timing sources
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 16 October 2006

#include "timing.h"

#include "prototype_timing.h"
#include "core/logging.h"

using logging::Level;

namespace timing
{
	RealType Timing::getPulseTimeError() const noexcept
	{
		return _enabled && _model ? _model->getSample() : 0.0f;
	}

	void Timing::skipSamples(const long long samples) const noexcept
	{
		if (_enabled && _model) { _model->skipSamples(samples); }
	}

	void Timing::initializeModel(const PrototypeTiming* timing)
	{
		if (!_alphas.empty())
		{
			throw std::logic_error("[BUG] ClockModelTiming::initializeModel called more than once");
		}

		timing->copyAlphas(_alphas, _weights);

		_model = std::make_unique<noise::ClockModelGenerator>(_alphas, _weights, timing->getFrequency(),
		                                                      timing->getPhaseOffset(), timing->getFreqOffset(), 15);

		if (timing->getFrequency() == 0.0f)
		{
			LOG(Level::INFO, "Timing source frequency not set, results could be incorrect.");
		}

		_frequency = timing->getFrequency();
		_sync_on_pulse = timing->getSyncOnPulse();
		_enabled = true;
	}
}
