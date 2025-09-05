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
		if (_model)
		{
			LOG(Level::WARNING, "Timing source '{}' already initialized. Skipping re-initialization.", _name);
			return;
		}

		// TODO: should each timing object source it's own mt19937 engine?
		_frequency = timing->getFrequency();

		_freq_offset = timing->getFreqOffset().value_or(0);
		if (const std::optional<RealType> random_freq_stdev = timing->getRandomFreqOffsetStdev(); random_freq_stdev)
		{
			LOG(Level::INFO, "Timing source '{}': applying random frequency offset with stdev {} Hz.", _name,
			    *random_freq_stdev);
			_freq_offset += noise::wgnSample(*random_freq_stdev);
		}

		_phase_offset = timing->getPhaseOffset().value_or(0);
		if (const std::optional<RealType> random_phase_stdev = timing->getRandomPhaseOffsetStdev(); random_phase_stdev)
		{
			LOG(Level::INFO, "Timing source '{}': applying random phase offset with stdev {} radians.", _name,
			    *random_phase_stdev);
			_phase_offset += noise::wgnSample(*random_phase_stdev);
		}

		timing->copyAlphas(_alphas, _weights);

		_model = std::make_unique<noise::ClockModelGenerator>(_alphas, _weights, _frequency, _phase_offset,
		                                                      _freq_offset, 15);

		if (timing->getFrequency() == 0.0f)
		{
			LOG(Level::INFO, "Timing source frequency not set, results could be incorrect.");
		}

		_sync_on_pulse = timing->getSyncOnPulse();
		_enabled = true;
	}
}
