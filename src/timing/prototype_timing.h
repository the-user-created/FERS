// prototype_timing.h
// Created by David Young on 9/17/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef PROTOTYPE_TIMING_H
#define PROTOTYPE_TIMING_H

#include <optional>
#include <string>
#include <vector>

#include "config.h"
#include "noise/noise_utils.h"

namespace timing
{
	class PrototypeTiming
	{
	public:
		explicit PrototypeTiming(std::string name) : _name(std::move(name)), _frequency(0), _sync_on_pulse(false) {}

		void addAlpha(RealType alpha, RealType weight);

		void getAlphas(std::vector<RealType>& getAlphas, std::vector<RealType>& getWeights) const;

		[[nodiscard]] RealType getPhaseOffset() const
		{
			return _random_phase.has_value() ? noise::wgnSample(*_random_phase) : _phase_offset.value_or(0);
		}

		[[nodiscard]] RealType getFreqOffset() const
		{
			return _random_freq.has_value() ? noise::wgnSample(*_random_freq) : _freq_offset.value_or(0);
		}

		[[nodiscard]] RealType getFrequency() const { return _frequency; }

		[[nodiscard]] bool getSyncOnPulse() const { return _sync_on_pulse; }

		void addFreqOffset(RealType offset);

		void addPhaseOffset(RealType offset);

		void addRandomFreqOffset(RealType stdev);

		void addRandomPhaseOffset(RealType stdev);

		void setFrequency(const RealType freq) { _frequency = freq; }

		[[nodiscard]] std::string getName() const { return _name; }

		void setSyncOnPulse() { _sync_on_pulse = true; }

	private:
		std::string _name;
		std::vector<RealType> _alphas;
		std::vector<RealType> _weights;
		std::optional<RealType> _freq_offset;
		std::optional<RealType> _phase_offset;
		std::optional<RealType> _random_phase;
		std::optional<RealType> _random_freq;
		RealType _frequency;
		bool _sync_on_pulse;
	};
}

#endif //PROTOTYPE_TIMING_H
