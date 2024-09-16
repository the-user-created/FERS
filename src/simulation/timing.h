// timing.h
// Timing source for simulation - all objects must be slaved to a timing source
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 16 October 2006

#ifndef TIMING_H
#define TIMING_H

#include <string>
#include <vector>
#include <boost/utility.hpp>

#include "config.h"
#include "noise_generators.h"

namespace rs
{
	class ClockModelGenerator;

	class Timing : boost::noncopyable
	{
	public:
		explicit Timing(std::string name) : _name(std::move(name))
		{
		}

		virtual ~Timing() = default;

		[[nodiscard]] virtual RS_FLOAT getPulseTimeError() const = 0;

		virtual RS_FLOAT nextNoiseSample() = 0;

		virtual void skipSamples(long long samples) = 0;

		[[nodiscard]] std::string getName() const  // TODO: unused
		{
			return _name;
		}

	private:
		std::string _name;
	};

	class PrototypeTiming
	{
	public:
		explicit PrototypeTiming(std::string name) : _name(std::move(name)), _freq_offset(0), _phase_offset(0),
		                                             _random_phase(0), _random_freq(0), _frequency(0),
		                                             _sync_on_pulse(false)
		{
		}

		void addAlpha(RS_FLOAT alpha, RS_FLOAT weight);

		void getAlphas(std::vector<RS_FLOAT>& getAlphas, std::vector<RS_FLOAT>& getWeights) const;

		[[nodiscard]] RS_FLOAT getPhaseOffset() const
		{
			return _random_phase != 0 ? rs_noise::wgnSample(_random_phase) : _phase_offset;
		}

		[[nodiscard]] RS_FLOAT getFreqOffset() const
		{
			return _random_freq != 0 ? rs_noise::wgnSample(_random_freq) : _freq_offset;
		}

		[[nodiscard]] RS_FLOAT getFrequency() const
		{
			return _frequency;
		}

		[[nodiscard]] bool getSyncOnPulse() const
		{
			return _sync_on_pulse;
		}

		void addFreqOffset(RS_FLOAT offset);

		void addPhaseOffset(RS_FLOAT offset);

		void addRandomFreqOffset(RS_FLOAT stdev);

		void addRandomPhaseOffset(RS_FLOAT stdev);

		void setFrequency(const RS_FLOAT freq)
		{
			_frequency = freq;
		}

		[[nodiscard]] std::string getName() const
		{
			return _name;
		}

		void setSyncOnPulse()
		{
			_sync_on_pulse = true;
		}

	private:
		std::string _name;
		std::vector<RS_FLOAT> _alphas;
		std::vector<RS_FLOAT> _weights;
		RS_FLOAT _freq_offset;
		RS_FLOAT _phase_offset;
		RS_FLOAT _random_phase;
		RS_FLOAT _random_freq;
		RS_FLOAT _frequency;
		bool _sync_on_pulse;
	};

	class ClockModelTiming final : public Timing
	{
	public:
		explicit ClockModelTiming(const std::string& name) : Timing(name), _enabled(false), _model(nullptr)
		{
		}

		~ClockModelTiming() override
		{
			delete _model;
		}

		RS_FLOAT nextNoiseSample() override
		{
			return _enabled ? _model->getSample() : 0;
		}

		void skipSamples(long long samples) override;

		void reset() const
		{
			_model->reset();
		}

		[[nodiscard]] bool getSyncOnPulse() const
		{
			return _sync_on_pulse;  // TODO: BUG #2 - _sync_on_pulse always false
		}

		void initializeModel(const PrototypeTiming* timing);

		[[nodiscard]] RS_FLOAT getPulseTimeError() const override  // TODO: unused
		{
			return _enabled ? _model->getSample() : 0;
		}

		[[nodiscard]] RS_FLOAT getFrequency() const
		{
			return _frequency;  // TODO: BUG #2 - _frequency always 0.0
		}

		[[nodiscard]] bool enabled() const
		{
			return _enabled && _model->enabled();
		}

	private:
		bool _enabled;
		ClockModelGenerator* _model;
		std::vector<RS_FLOAT> _alphas;
		std::vector<RS_FLOAT> _weights;
		RS_FLOAT _frequency{};  // TODO: BUG #1 - default-initialization of _frequency
		bool _sync_on_pulse{};  // TODO: BUG #2 - default-initialization of _sync_on_pulse
	};
}

#endif
