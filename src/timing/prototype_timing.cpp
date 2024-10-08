/**
 * @file prototype_timing.cpp
 * @brief Implementation file for the PrototypeTiming class.
 *
 * @authors David Young, Marc Brooker
 * @date 2024-09-17
 */

#include "prototype_timing.h"

#include "core/logging.h"
#include "noise/noise_utils.h"

using logging::Level;

namespace timing
{
	RealType PrototypeTiming::getPhaseOffset() const noexcept
	{
		return _random_phase ? noise::wgnSample(*_random_phase) : _phase_offset.value_or(0);
	}

	RealType PrototypeTiming::getFreqOffset() const noexcept
	{
		return _random_freq ? noise::wgnSample(*_random_freq) : _freq_offset.value_or(0);
	}

	void PrototypeTiming::setAlpha(const RealType alpha, const RealType weight) noexcept
	{
		_alphas.emplace_back(alpha);
		_weights.emplace_back(weight);
	}

	void PrototypeTiming::copyAlphas(std::vector<RealType>& alphas, std::vector<RealType>& weights) const noexcept
	{
		alphas = _alphas;
		weights = _weights;
	}

	void PrototypeTiming::setFreqOffset(const RealType offset) noexcept
	{
		if (_random_freq) { logOffsetConflict("frequency"); }
		_freq_offset = offset;
	}

	void PrototypeTiming::setPhaseOffset(const RealType offset) noexcept
	{
		if (_random_phase) { logOffsetConflict("phase"); }
		_phase_offset = offset;
	}

	void PrototypeTiming::setRandomFreqOffset(const RealType stdev) noexcept
	{
		if (_freq_offset) { logOffsetConflict("frequency"); }
		_random_freq = stdev;
	}

	void PrototypeTiming::setRandomPhaseOffset(const RealType stdev) noexcept
	{
		if (_phase_offset) { logOffsetConflict("phase"); }
		_random_phase = stdev;
	}

	void PrototypeTiming::logOffsetConflict(const std::string& offsetType) const noexcept
	{
		LOG(Level::ERROR,
		    "Random {0} offset and constant {0} offset are set for timing source {1}. Only the random offset will be used.",
		    offsetType.c_str(), _name.c_str());
	}
}
