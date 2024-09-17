// timing.h
// Timing source for simulation - all objects must be slaved to a timing source
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 16 October 2006

#ifndef TIMING_H
#define TIMING_H

#include <optional>
#include <string>
#include <vector>

#include "config.h"
#include "noise_generators.h"

namespace rs
{
	class ClockModelGenerator;

	// TODO: This should be moved to a separate file
	class PrototypeTiming
	{
	public:
		explicit PrototypeTiming(std::string name) : _name(std::move(name)), _frequency(0), _sync_on_pulse(false) {}

		void addAlpha(RS_FLOAT alpha, RS_FLOAT weight);

		void getAlphas(std::vector<RS_FLOAT>& getAlphas, std::vector<RS_FLOAT>& getWeights) const;

		[[nodiscard]] RS_FLOAT getPhaseOffset() const
		{
			return _random_phase.has_value() ? rs_noise::wgnSample(*_random_phase) : _phase_offset.value_or(0);
		}

		[[nodiscard]] RS_FLOAT getFreqOffset() const
		{
			return _random_freq.has_value() ? rs_noise::wgnSample(*_random_freq) : _freq_offset.value_or(0);
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
