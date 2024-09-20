// timing.h
// Timing source for simulation - all objects must be slaved to a timing source
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 16 October 2006

#ifndef TIMING_H
#define TIMING_H

#include <string>
#include <vector>

#include "config.h"
#include "noise/noise_generators.h"

namespace rs
{
	class PrototypeTiming;
	class ClockModelGenerator;

	class Timing final
	{
	public:
		explicit Timing(std::string name)
			: _name(std::move(name)), _enabled(false), _model(nullptr) {}

		// Virtual destructor ensures proper cleanup in derived classes
		~Timing() = default;

		// Deleted copy constructor and assignment operator to prevent copying
		Timing(const Timing&) = delete;

		Timing& operator=(const Timing&) = delete;

		[[nodiscard]] RS_FLOAT getPulseTimeError() const noexcept
		{
			return _enabled && _model ? _model->getSample() : 0.0f;
		}

		[[nodiscard]] RS_FLOAT nextNoiseSample() const { return _enabled ? _model->getSample() : 0.0f; }

		void skipSamples(const long long samples) const { if (_enabled && _model) { _model->skipSamples(samples); } }

		[[nodiscard]] std::string getName() const noexcept { return _name; }

		[[nodiscard]] bool getSyncOnPulse() const noexcept { return _sync_on_pulse; }

		void initializeModel(const PrototypeTiming* timing);

		void reset() const { if (_model) { _model->reset(); } }

		[[nodiscard]] RS_FLOAT getFrequency() const noexcept { return _frequency; }

		[[nodiscard]] bool enabled() const noexcept { return _enabled && _model && _model->enabled(); }

	private:
		std::string _name;
		bool _enabled;
		std::unique_ptr<ClockModelGenerator> _model; // Use a smart pointer for automatic memory management
		std::vector<RS_FLOAT> _alphas;
		std::vector<RS_FLOAT> _weights;
		RS_FLOAT _frequency{};
		bool _sync_on_pulse{};
	};
}

#endif
