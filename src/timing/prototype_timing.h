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

		void addAlpha(RS_FLOAT alpha, RS_FLOAT weight);

		void getAlphas(std::vector<RS_FLOAT>& getAlphas, std::vector<RS_FLOAT>& getWeights) const;

		[[nodiscard]] RS_FLOAT getPhaseOffset() const
		{
			return _random_phase.has_value() ? noise::wgnSample(*_random_phase) : _phase_offset.value_or(0);
		}

		[[nodiscard]] RS_FLOAT getFreqOffset() const
		{
			return _random_freq.has_value() ? noise::wgnSample(*_random_freq) : _freq_offset.value_or(0);
		}

		[[nodiscard]] RS_FLOAT getFrequency() const { return _frequency; }

		[[nodiscard]] bool getSyncOnPulse() const { return _sync_on_pulse; }

		void addFreqOffset(RS_FLOAT offset);

		void addPhaseOffset(RS_FLOAT offset);

		void addRandomFreqOffset(RS_FLOAT stdev);

		void addRandomPhaseOffset(RS_FLOAT stdev);

		void setFrequency(const RS_FLOAT freq) { _frequency = freq; }

		[[nodiscard]] std::string getName() const { return _name; }

		void setSyncOnPulse() { _sync_on_pulse = true; }

	private:
		std::string _name;
		std::vector<RS_FLOAT> _alphas;
		std::vector<RS_FLOAT> _weights;
		std::optional<RS_FLOAT> _freq_offset;
		std::optional<RS_FLOAT> _phase_offset;
		std::optional<RS_FLOAT> _random_phase;
		std::optional<RS_FLOAT> _random_freq;
		RS_FLOAT _frequency;
		bool _sync_on_pulse;
	};
}

#endif //PROTOTYPE_TIMING_H
