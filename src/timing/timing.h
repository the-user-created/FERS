// timing.h
// Timing source for simulation - all objects must be slaved to a timing source
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 16 October 2006

#ifndef TIMING_H
#define TIMING_H

#include <memory>                    // for unique_ptr
#include <string>                    // for string
#include <utility>                   // for move
#include <vector>                    // for vector

#include "config.h"                  // for RealType
#include "noise/noise_generators.h"  // for ClockModelGenerator

namespace timing
{
	class PrototypeTiming;

	class Timing final
	{
	public:
		explicit Timing(std::string name) : _name(std::move(name)) {}

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

		[[nodiscard]] RealType getPulseTimeError() const noexcept;
		void skipSamples(long long samples) const noexcept;

		void initializeModel(const PrototypeTiming* timing);

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

#endif
