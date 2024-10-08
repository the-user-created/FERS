/**
 * @file timing.cpp
 * @brief Implementation of timing sources.
 *
 * @authors David Young, Marc Brooker
 * @date 2006-10-16
 */

#include "timing.h"

#include "prototype_timing.h"
#include "core/logging.h"

using logging::Level;

namespace timing
{
	void Timing::skipSamples(const long long samples) const noexcept
	{
		if (_enabled && _model) { _model->skipSamples(samples); }
	}

	void Timing::initializeModel(const PrototypeTiming* timing) noexcept
	{
		if (!_alphas.empty()) { LOG(Level::ERROR, "Timing source already initialized."); }

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
