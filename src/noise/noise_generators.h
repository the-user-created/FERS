// noise_generators.h
// Functions and classes to generate noise of various types
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 14 August 2006

#ifndef NOISE_GENERATORS_H
#define NOISE_GENERATORS_H

#include <memory>                     // for unique_ptr
#include <random>                     // for normal_distribution, default_ra...
#include <string>                     // for char_traits, string
#include <vector>                     // for vector

#include "config.h"                   // for RealType
#include "falpha_branch.h"            // for FAlphaBranch
#include "python/python_extension.h"  // for PythonNoise

namespace noise
{
	class NoiseGenerator
	{
	public:
		NoiseGenerator() = default;

		virtual ~NoiseGenerator() = default;

		// Pure virtual method to generate a sample
		virtual RealType getSample() = 0;

		NoiseGenerator(const NoiseGenerator&) = delete;

		NoiseGenerator& operator=(const NoiseGenerator&) = delete;
	};

	class WgnGenerator final : public NoiseGenerator
	{
	public:
		explicit WgnGenerator(RealType stddev = 1.0);

		RealType getSample() override { return _dist(_rng) * _stddev; }

	private:
		std::default_random_engine _rng;
		std::normal_distribution<> _dist{0.0, 1.0};
		RealType _stddev;
	};

	class GammaGenerator final : public NoiseGenerator
	{
	public:
		explicit GammaGenerator(RealType k);

		RealType getSample() override { return _dist(_rng); }

	private:
		std::default_random_engine _rng;
		std::gamma_distribution<> _dist;
	};

	class MultirateGenerator final : public NoiseGenerator
	{
	public:
		MultirateGenerator(RealType alpha, unsigned branches);

		RealType getSample() override { return _topbranch->getSample() * _scale; }

		void skipSamples(long long samples) const;

		void reset() const;

	private:
		RealType _scale;
		std::unique_ptr<FAlphaBranch> _topbranch;

		void createTree(RealType fAlpha, int fInt, unsigned branches);
	};

	class ClockModelGenerator final : public NoiseGenerator
	{
	public:
		ClockModelGenerator(const std::vector<RealType>& alpha, const std::vector<RealType>& inWeights,
		                    RealType frequency, RealType phaseOffset, RealType freqOffset, int branches);

		RealType getSample() override;

		void skipSamples(long long samples);

		void reset();

		[[nodiscard]] bool enabled() const;

	private:
		std::vector<std::unique_ptr<MultirateGenerator>> _generators;
		std::vector<RealType> _weights;
		RealType _phase_offset;
		RealType _freq_offset;
		RealType _frequency;
		unsigned long _count = 0;
	};

	// NOTE: This class is not used
	class PythonNoiseGenerator final : public NoiseGenerator
	{
	public:
		PythonNoiseGenerator(const std::string& module, const std::string& function) : _generator(module, function) {}

		RealType getSample() override { return _generator.getSample(); }

	private:
		python::PythonNoise _generator;
	};
}

#endif
