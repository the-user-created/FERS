// noise_generators.h
// Functions and classes to generate noise of various types
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 14 August 2006

#ifndef NOISE_GENERATORS_H
#define NOISE_GENERATORS_H

#include <memory>
#include <vector>
#include <boost/random.hpp>
#include <boost/utility.hpp>

#include "config.h"
#include "math_utils/dsp_filters.h"
#include "python/python_extension.h"

namespace rs_noise
{
	void initializeNoise();

	void cleanUpNoise();

	RS_FLOAT wgnSample(RS_FLOAT stddev);

	RS_FLOAT uniformSample();

	RS_FLOAT noiseTemperatureToPower(RS_FLOAT temperature, RS_FLOAT bandwidth);
}

namespace rs
{
	class NoiseGenerator : boost::noncopyable
	{
	public:
		NoiseGenerator() = default;

		virtual ~NoiseGenerator() = default;

		virtual RS_FLOAT getSample() = 0;
	};

	class WgnGenerator final : public NoiseGenerator
	{
	public:
		explicit WgnGenerator(RS_FLOAT stddev);

		WgnGenerator();

		~WgnGenerator() override
		{
			delete _gen;
		}

		RS_FLOAT getSample() override
		{
			return (*_gen)();
		}

	private:
		boost::normal_distribution<> _dist;
		boost::variate_generator<boost::mt19937&, boost::normal_distribution<>>* _gen;
		RS_FLOAT _temperature{};
	};

	class GammaGenerator final : public NoiseGenerator
	{
	public:
		explicit GammaGenerator(RS_FLOAT k);

		~GammaGenerator() override = default;

		RS_FLOAT getSample() override
		{
			return _gen();
		}

		RS_FLOAT operator()()
		{
			return _gen();
		}

	private:
		boost::gamma_distribution<> _dist;
		boost::variate_generator<boost::mt19937&, boost::gamma_distribution<>> _gen;
	};

	// TODO: Move FAlphaBranch to a separate file?
	class FAlphaBranch : boost::noncopyable
	{
	public:
		FAlphaBranch(RS_FLOAT ffrac, unsigned fint, FAlphaBranch* pre, bool last);

		~FAlphaBranch();

		RS_FLOAT getSample();

		void flush(RS_FLOAT scale);

	private:
		void init();

		void clean() const;

		void refill();

		RS_FLOAT calcSample();

		IirFilter* _shape_filter{};
		RS_FLOAT _shape_gain{};
		IirFilter* _integ_filter{};
		RS_FLOAT _integ_gain{};
		RS_FLOAT _upsample_scale;
		IirFilter* _highpass{};
		FAlphaBranch* _pre;
		bool _last;
		DecadeUpsampler* _upsampler{};
		RS_FLOAT* _buffer;
		unsigned _buffer_samples{};
		RS_FLOAT _ffrac;
		RS_FLOAT _fint;
		RS_FLOAT _offset_sample{};
		bool _got_offset{};
		RS_FLOAT _pre_scale{};
		friend class MultirateGenerator;
	};

	class MultirateGenerator final : public NoiseGenerator
	{
	public:
		MultirateGenerator(RS_FLOAT alpha, unsigned branches);

		~MultirateGenerator() override
		{
			delete _topbranch;
		}

		RS_FLOAT getSample() override
		{
			return _topbranch->getSample() * _scale;
		}

		void skipSamples(long long samples) const;

		void reset() const;

	private:
		RS_FLOAT _scale;

		void createTree(RS_FLOAT falpha, int fint, unsigned branches);

		FAlphaBranch* _topbranch{};
	};

	class ClockModelGenerator final : public NoiseGenerator
	{
	public:
		ClockModelGenerator(const std::vector<RS_FLOAT>& alpha, const std::vector<RS_FLOAT>& inWeights,
		                    RS_FLOAT frequency, RS_FLOAT phaseOffset, RS_FLOAT freqOffset, int branches);

		~ClockModelGenerator() override = default;

		RS_FLOAT getSample() override;

		void skipSamples(long long samples);

		void reset();

		[[nodiscard]] bool enabled() const
		{
			return !_generators.empty() || _freq_offset != 0 || _phase_offset != 0;
		}

	private:
		std::vector<std::unique_ptr<MultirateGenerator>> _generators;
		std::vector<RS_FLOAT> _weights;
		RS_FLOAT _phase_offset;
		RS_FLOAT _freq_offset;
		RS_FLOAT _frequency;
		unsigned long _count;
	};

	class PythonNoiseGenerator final : public NoiseGenerator
	{
	public:
		PythonNoiseGenerator(const std::string& module, const std::string& function) : _generator(module, function)
		{
		}

		~PythonNoiseGenerator() override = default;

		RS_FLOAT getSample() override
		{
			return _generator.getSample();
		}

	private:
		rs_python::PythonNoise _generator;
	};
}

#endif
