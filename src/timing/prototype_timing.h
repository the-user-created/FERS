// prototype_timing.h
// Created by David Young on 9/17/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "config.h"

namespace timing
{
	class PrototypeTiming
	{
	public:
		explicit PrototypeTiming(std::string name) : _name(std::move(name)) {}

		~PrototypeTiming() = default;

		void copyAlphas(std::vector<RealType>& alphas, std::vector<RealType>& weights) const;

		[[nodiscard]] RealType getFrequency() const noexcept { return _frequency; }
		[[nodiscard]] std::string getName() const { return _name; }
		[[nodiscard]] bool getSyncOnPulse() const noexcept { return _sync_on_pulse; }

		[[nodiscard]] RealType getPhaseOffset() const noexcept;

		[[nodiscard]] RealType getFreqOffset() const noexcept;

		void setFrequency(const RealType freq) noexcept { _frequency = freq; }
		void setSyncOnPulse() noexcept { _sync_on_pulse = true; }

		void setAlpha(RealType alpha, RealType weight);

		void setFreqOffset(RealType offset);

		void setPhaseOffset(RealType offset);

		void setRandomFreqOffset(RealType stdev);

		void setRandomPhaseOffset(RealType stdev);

	private:
		std::string _name;
		std::vector<RealType> _alphas;
		std::vector<RealType> _weights;
		std::optional<RealType> _freq_offset;
		std::optional<RealType> _phase_offset;
		std::optional<RealType> _random_phase;
		std::optional<RealType> _random_freq;
		RealType _frequency{0};
		bool _sync_on_pulse{false};

		void logOffsetConflict(const std::string& offsetType) const;
	};
}
