// noise_generators.h
// Functions and classes to generate noise of various types
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 14 August 2006

#ifndef NOISE_GENERATORS_H
#define NOISE_GENERATORS_H

#include <memory>
#include <random>

#include "falpha_branch.h"
#include "python/python_extension.h"

namespace noise
{
	class NoiseGenerator
	{
	public:
		NoiseGenerator() = default;

		virtual ~NoiseGenerator() = default;

		// Pure virtual method to generate a sample
		virtual RealType getSample() = 0;

		// Delete copy constructor and assignment operator to make the class noncopyable
		NoiseGenerator(const NoiseGenerator&) = delete;

		NoiseGenerator& operator=(const NoiseGenerator&) = delete;
	};

	class WgnGenerator final : public NoiseGenerator
	{
	public:
		explicit WgnGenerator(RealType stddev);

		// Default constructor with standard deviation = 1.0
		WgnGenerator();

		~WgnGenerator() override = default;

		// Override to generate a sample
		double getSample() override { return _dist(*_rng) * _stddev; }

	private:
		std::unique_ptr<std::mt19937> _rng;
		std::normal_distribution<> _dist;
		RealType _stddev{};
	};

	class GammaGenerator final : public NoiseGenerator
	{
	public:
		explicit GammaGenerator(RealType k);

		~GammaGenerator() override = default;

		// Override to generate a sample
		RealType getSample() override { return _dist(*_rng); }

		// Functor interface
		RealType operator()() { return _dist(*_rng); }

	private:
		std::unique_ptr<std::mt19937> _rng;
		std::gamma_distribution<> _dist;
	};

	class MultirateGenerator final : public NoiseGenerator
	{
	public:
		MultirateGenerator(RealType alpha, unsigned branches);

		~MultirateGenerator() override = default;

		RealType getSample() override { return _topbranch->getSample() * _scale; }

		void skipSamples(long long samples) const;

		void reset() const;

	private:
		RealType _scale;

		void createTree(RealType fAlpha, int fInt, unsigned branches);

		std::unique_ptr<FAlphaBranch> _topbranch;
	};

	class ClockModelGenerator final : public NoiseGenerator
	{
	public:
		ClockModelGenerator(const std::vector<RealType>& alpha, const std::vector<RealType>& inWeights,
		                    RealType frequency, RealType phaseOffset, RealType freqOffset, int branches);

		~ClockModelGenerator() override = default;

		RealType getSample() override;

		void skipSamples(long long samples);

		void reset();

		[[nodiscard]] bool enabled() const { return !_generators.empty() || _freq_offset != 0 || _phase_offset != 0; }

	private:
		std::vector<std::unique_ptr<MultirateGenerator>> _generators;
		std::vector<RealType> _weights;
		RealType _phase_offset;
		RealType _freq_offset;
		RealType _frequency;
		unsigned long _count{0}; // Initialize with 0
	};

	class PythonNoiseGenerator final : public NoiseGenerator
	{
	public:
		PythonNoiseGenerator(const std::string& module, const std::string& function) : _generator(module, function) {}

		~PythonNoiseGenerator() override = default;

		// Note: This function is not used in the codebase
		RealType getSample() override { return _generator.getSample(); }

	private:
		python::PythonNoise _generator;
	};
}

#endif
