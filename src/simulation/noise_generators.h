// noise_generators.h
// Functions and classes to generate noise of various types
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 14 August 2006

#ifndef NOISE_GENERATORS_H
#define NOISE_GENERATORS_H

#include <memory>
#include <random>
#include <vector>

#include "config.h"
#include "math_utils/dsp_filters.h"
#include "python/python_extension.h"

namespace rs_noise
{
	void initializeNoise();

	void cleanUpNoise();

	RS_FLOAT wgnSample(RS_FLOAT stddev);

	// Note: This function is not used in the codebase
	RS_FLOAT uniformSample();

	RS_FLOAT noiseTemperatureToPower(RS_FLOAT temperature, RS_FLOAT bandwidth);
}

namespace rs
{
	class NoiseGenerator
	{
	public:
		NoiseGenerator() = default;

		virtual ~NoiseGenerator() = default;

		// Pure virtual method to generate a sample
		virtual RS_FLOAT getSample() = 0;

		// Delete copy constructor and assignment operator to make the class noncopyable
		NoiseGenerator(const NoiseGenerator&) = delete;

		NoiseGenerator& operator=(const NoiseGenerator&) = delete;
	};

	class WgnGenerator final : public NoiseGenerator
	{
	public:
		explicit WgnGenerator(RS_FLOAT stddev);

		// Default constructor with standard deviation = 1.0
		WgnGenerator();

		~WgnGenerator() override = default;

		// Override to generate a sample
		double getSample() override { return _dist(*_rng) * _stddev; }

	private:
		std::unique_ptr<std::mt19937> _rng;
		std::normal_distribution<> _dist;
		RS_FLOAT _stddev{};
	};

	class GammaGenerator final : public NoiseGenerator
	{
	public:
		explicit GammaGenerator(RS_FLOAT k);

		~GammaGenerator() override = default;

		// Override to generate a sample
		RS_FLOAT getSample() override { return _dist(*_rng); }

		// Functor interface
		RS_FLOAT operator()() { return _dist(*_rng); }

	private:
		std::unique_ptr<std::mt19937> _rng;
		std::gamma_distribution<> _dist;
	};

	// TODO: move to a separate file
	class FAlphaBranch
	{
	public:
		FAlphaBranch(RS_FLOAT ffrac, unsigned fint, std::unique_ptr<FAlphaBranch> pre, bool last);

		~FAlphaBranch() { clean(); }

		// Delete copy constructor and assignment operator to make the class noncopyable
		FAlphaBranch(const FAlphaBranch&) = delete;

		FAlphaBranch& operator=(const FAlphaBranch&) = delete;

		RS_FLOAT getSample();

		void flush(RS_FLOAT scale);

		[[nodiscard]] FAlphaBranch* getPre() const { return _pre.get(); }

	private:
		void init();

		void clean();

		void refill();

		RS_FLOAT calcSample();

		std::unique_ptr<IirFilter> _shape_filter;
		RS_FLOAT _shape_gain{};
		std::unique_ptr<IirFilter> _integ_filter;
		RS_FLOAT _integ_gain{};
		RS_FLOAT _upsample_scale;
		std::unique_ptr<IirFilter> _highpass;
		std::unique_ptr<FAlphaBranch> _pre;
		bool _last;
		std::unique_ptr<DecadeUpsampler> _upsampler;
		std::unique_ptr<RS_FLOAT[]> _buffer;
		unsigned _buffer_samples{};
		RS_FLOAT _ffrac;
		RS_FLOAT _fint;
		RS_FLOAT _offset_sample{};
		bool _got_offset{};
		RS_FLOAT _pre_scale{};
	};

	class MultirateGenerator final : public NoiseGenerator
	{
	public:
		MultirateGenerator(RS_FLOAT alpha, unsigned branches);

		~MultirateGenerator() override = default;

		RS_FLOAT getSample() override { return _topbranch->getSample() * _scale; }

		void skipSamples(long long samples) const;

		void reset() const;

	private:
		RS_FLOAT _scale;

		void createTree(RS_FLOAT fAlpha, int fInt, unsigned branches);

		std::unique_ptr<FAlphaBranch> _topbranch;
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

		[[nodiscard]] bool enabled() const { return !_generators.empty() || _freq_offset != 0 || _phase_offset != 0; }

	private:
		std::vector<std::unique_ptr<MultirateGenerator>> _generators;
		std::vector<RS_FLOAT> _weights;
		RS_FLOAT _phase_offset;
		RS_FLOAT _freq_offset;
		RS_FLOAT _frequency;
		unsigned long _count{0}; // Initialize with 0
	};

	class PythonNoiseGenerator final : public NoiseGenerator
	{
	public:
		PythonNoiseGenerator(const std::string& module, const std::string& function) : _generator(module, function) {}

		~PythonNoiseGenerator() override = default;

		// Note: This function is not used in the codebase
		RS_FLOAT getSample() override { return _generator.getSample(); }

	private:
		rs_python::PythonNoise _generator;
	};
}

#endif
