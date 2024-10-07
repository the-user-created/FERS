// timing.h
// Timing source for simulation - all objects must be slaved to a timing source
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 16 October 2006

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "config.h"
#include "noise/noise_generators.h"

namespace timing
{
	class PrototypeTiming;

	class Timing final
	{
	public:
		explicit Timing(std::string name) noexcept : _name(std::move(name)) {}

		// Defaulted destructor is sufficient
		~Timing() = default;

		// Deleted copy constructor and assignment operator to prevent copying
		Timing(const Timing&) = delete;

		Timing& operator=(const Timing&) = delete;

		[[nodiscard]] RealType getNextSample() const noexcept { return _enabled ? _model->getSample() : 0.0f; }
		[[nodiscard]] std::string getName() const noexcept { return _name; }
		[[nodiscard]] bool getSyncOnPulse() const noexcept { return _sync_on_pulse; }
		[[nodiscard]] RealType getFrequency() const noexcept { return _frequency; }
		[[nodiscard]] bool isEnabled() const noexcept { return _enabled && _model && _model->enabled(); }

		// Note: This function is not used in the codebase
		/*[[nodiscard]] RealType getPulseTimeError() const noexcept {
			return _enabled && _model ? _model->getSample() : 0.0f;
		}*/
		void skipSamples(long long samples) const noexcept;

		void initializeModel(const PrototypeTiming* timing) noexcept;

		void reset() const noexcept { if (_model) { _model->reset(); } }

	private:
		std::string _name;
		bool _enabled{false};
		std::unique_ptr<noise::ClockModelGenerator> _model{nullptr};
		std::vector<RealType> _alphas;
		std::vector<RealType> _weights;
		RealType _frequency{};
		bool _sync_on_pulse{false};
	};
}
