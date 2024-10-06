// noise_generators.h
// Functions and classes to generate noise of various types
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 14 August 2006

#pragma once

#include <memory>                     // for unique_ptr
#include <random>                     // for normal_distribution, default_ra...
#include <vector>                     // for vector

#include "config.h"                   // for RealType
#include "falpha_branch.h"            // for FAlphaBranch

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
}
