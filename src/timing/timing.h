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

	class Timing
	{
	public:
		explicit Timing(std::string name) : _name(std::move(name)) {}

		// Virtual destructor ensures proper cleanup in derived classes
		virtual ~Timing() = default;

		// Deleted copy constructor and assignment operator to prevent copying
		Timing(const Timing&) = delete;
		Timing& operator=(const Timing&) = delete;

		[[nodiscard]] virtual RS_FLOAT getPulseTimeError() const noexcept = 0;

		virtual RS_FLOAT nextNoiseSample() = 0;

		virtual void skipSamples(long long samples) = 0;

		// Note: This function is not used in the codebase
		[[nodiscard]] std::string getName() const noexcept { return _name; }

	private:
		std::string _name;
	};

	class ClockModelTiming final : public Timing
	{
	public:
		explicit ClockModelTiming(const std::string& name) : Timing(name), _enabled(false), _model(nullptr) {}

		~ClockModelTiming() override = default;

		RS_FLOAT nextNoiseSample() override { return _enabled ? _model->getSample() : 0.0f; }

		void skipSamples(const long long samples) override { if (_enabled && _model) { _model->skipSamples(samples); } }

		void reset() const { if (_model) { _model->reset(); } }

		[[nodiscard]] bool getSyncOnPulse() const noexcept { return _sync_on_pulse; }

		void initializeModel(const PrototypeTiming* timing);

		// Note: This function is not used in the codebase
		[[nodiscard]] RS_FLOAT getPulseTimeError() const noexcept override
		{
			return _enabled && _model ? _model->getSample() : 0.0f;
		}

		[[nodiscard]] RS_FLOAT getFrequency() const noexcept { return _frequency; }

		[[nodiscard]] bool enabled() const noexcept { return _enabled && _model && _model->enabled(); }

	private:
		bool _enabled;
		std::unique_ptr<ClockModelGenerator> _model; // Use a smart pointer for automatic memory management
		std::vector<RS_FLOAT> _alphas;
		std::vector<RS_FLOAT> _weights;
		RS_FLOAT _frequency{};
		bool _sync_on_pulse{};
	};
}

#endif
