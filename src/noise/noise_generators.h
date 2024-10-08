/**
* @file noise_generators.h
* @brief Header file for noise generator classes.
*
* This file contains the declaration of several noise generator classes,
* each providing different noise generation techniques.
* The noise types include white Gaussian noise (WGN), Gamma noise, multirate noise, and clock model noise.
* These classes are used to produce random noise samples for various simulation or modeling purposes.
*
* @authors David Young, Marc Brooker
* @date 2006-08-14
*/

#pragma once

#include <memory>
#include <random>
#include <vector>

#include "config.h"
#include "falpha_branch.h"

namespace noise
{
	/**
	* @class NoiseGenerator
	* @brief Abstract base class for noise generators.
	*
	* This class serves as an interface for all noise generators.
	* It provides a pure virtual method `getSample()`
	* that must be implemented by any derived class to return a noise sample.
	*/
	class NoiseGenerator
	{
	public:
		/**
		* @brief Default constructor.
		*/
		NoiseGenerator() = default;

		/**
		* @brief Virtual destructor.
		*/
		virtual ~NoiseGenerator() = default;

		/**
		* @brief Pure virtual method to generate a noise sample.
		*
		* This method should be overridden by derived classes to return a specific type of noise sample.
		*
		* @return A noise sample of type RealType.
		*/
		virtual RealType getSample() = 0;

		NoiseGenerator(const NoiseGenerator&) = delete;

		NoiseGenerator& operator=(const NoiseGenerator&) = delete;
	};

	/**
	* @class WgnGenerator
	* @brief Generates white Gaussian noise.
	*
	* This class generates white Gaussian noise using a normal distribution.
	* The user can specify the standard deviation
	* of the noise.
	*/
	class WgnGenerator final : public NoiseGenerator
	{
	public:
		/**
		* @brief Constructor to initialize the WGN generator with a given standard deviation.
		*
		* @param stddev The standard deviation of the generated Gaussian noise. Default is 1.0.
		*/
		explicit WgnGenerator(RealType stddev = 1.0) noexcept;

		/**
		* @brief Generates a sample of white Gaussian noise.
		*
		* @return A noise sample of type RealType.
		*/
		RealType getSample() noexcept override { return _dist(_rng) * _stddev; }

	private:
		std::default_random_engine _rng; ///< Random number generator.
		std::normal_distribution<> _dist{0.0, 1.0}; ///< Normal distribution for generating Gaussian noise.
		RealType _stddev; ///< Standard deviation of the generated noise.
	};

	/**
	* @class GammaGenerator
	* @brief Generates Gamma-distributed noise.
	*
	* This class generates noise based on a Gamma distribution. The shape parameter k defines the distribution.
	*/
	class GammaGenerator final : public NoiseGenerator
	{
	public:
		/**
		* @brief Constructor to initialize the Gamma generator with a shape parameter.
		*
		* @param k The shape parameter of the Gamma distribution.
		*/
		explicit GammaGenerator(RealType k) noexcept;

		/**
		* @brief Generates a sample of Gamma noise.
		*
		* @return A noise sample of type RealType.
		*/
		RealType getSample() noexcept override { return _dist(_rng); }

	private:
		std::default_random_engine _rng; ///< Random number generator.
		std::gamma_distribution<> _dist; ///< Gamma distribution for generating noise.
	};

	/**
	* @class MultirateGenerator
	* @brief Generates multirate noise using a hierarchical tree structure.
	*
	* This class generates noise using multiple rate processes based on a fractional alpha branching structure.
	* It can simulate a range of noise processes.
	*/
	class MultirateGenerator final : public NoiseGenerator
	{
	public:
		/**
		* @brief Constructor to initialize the multirate generator.
		*
		* @param alpha The scaling parameter that controls the noise properties.
		* @param branches The number of branches in the tree structure.
		*/
		MultirateGenerator(RealType alpha, unsigned branches);

		/**
		* @brief Generates a multirate noise sample.
		*
		* @return A noise sample of type RealType.
		*/
		RealType getSample() override { return _topbranch->getSample() * _scale; }

		/**
		* @brief Skips a number of samples in the noise sequence.
		*
		* @param samples The number of samples to skip.
		*/
		void skipSamples(long long samples) const noexcept;

		/**
		* @brief Resets the noise generator state.
		*/
		void reset() const noexcept;

	private:
		RealType _scale; ///< Scaling factor for the noise.
		std::unique_ptr<FAlphaBranch> _topbranch; ///< Pointer to the top branch in the tree structure.

		/**
		* @brief Helper method to create the hierarchical tree structure.
		*
		* @param fAlpha Fractional alpha value.
		* @param fInt Integer part of the scaling factor.
		* @param branches The number of branches in the tree.
		*/
		void createTree(RealType fAlpha, int fInt, unsigned branches);
	};

	/**
	* @class ClockModelGenerator
	* @brief Generates noise using a clock model with multiple rates.
	*
	* This class models a noise process using multiple rate processes, where each rate process is weighted and combined
	* to generate the final noise output.
	* This is useful for simulating clock jitter or other similar phenomena.
	*/
	class ClockModelGenerator final : public NoiseGenerator
	{
	public:
		/**
		* @brief Constructor to initialize the clock model generator.
		*
		* @param alpha Vector of scaling parameters for the noise.
		* @param inWeights Vector of weights for each rate process.
		* @param frequency The base frequency of the clock model.
		* @param phaseOffset The phase offset of the generated noise.
		* @param freqOffset The frequency offset of the generated noise.
		* @param branches The number of branches in each rate process.
		*/
		ClockModelGenerator(const std::vector<RealType>& alpha, const std::vector<RealType>& inWeights,
		                    RealType frequency, RealType phaseOffset, RealType freqOffset, int branches) noexcept;

		/**
		* @brief Generates a clock model noise sample.
		*
		* @return A noise sample of type RealType.
		*/
		RealType getSample() override;

		/**
		* @brief Skips a number of samples in the noise sequence.
		*
		* @param samples The number of samples to skip.
		*/
		void skipSamples(long long samples);

		/**
		* @brief Resets the noise generator state.
		*/
		void reset();

		/**
		* @brief Checks if the noise generator is enabled.
		*
		* @return True if the generator is enabled, false otherwise.
		*/
		[[nodiscard]] bool enabled() const;

	private:
		std::vector<std::unique_ptr<MultirateGenerator>> _generators; ///< Multirate noise generators.
		std::vector<RealType> _weights; ///< Weights for each rate process.
		RealType _phase_offset; ///< Phase offset for the noise.
		RealType _freq_offset; ///< Frequency offset for the noise.
		RealType _frequency; ///< Base frequency of the clock model.
		unsigned long _count = 0; ///< Sample counter.
	};
}
