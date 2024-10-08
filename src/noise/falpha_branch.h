/**
* @file falpha_branch.h
* @brief Implementation of the FAlphaBranch class for noise generation.
*
* This file contains the definition of the FAlphaBranch class, which is responsible for generating noise
* using a fractional integrator filter.
* It includes methods for handling noise generation across multiple stages using upsampling,
* filtering, and recursive sample calculations.
* The class is designed to handle fractional and integer noise components
* and provides a mechanism for high-pass filtering and shaping the generated noise.
*
* @authors David Young, Marc Brooker
* @date 2024-09-17
*/

#pragma once

#include <memory>
#include <vector>

#include "config.h"
#include "signal/dsp_filters.h"

namespace noise
{
	/**
    * @class FAlphaBranch
    * @brief Class responsible for generating fractional and integer noise components.
    *
    * The FAlphaBranch class generates noise by applying a fractional integrator filter. It uses a series of
    * filters and upsamplers to process and shape the noise signal. This class is non-copyable and works recursively
    * to handle multiple stages of noise processing. It includes high-pass filtering, integration, and
    * fractional shaping for various types of noise generation.
    */
	class FAlphaBranch
	{
	public:
		/**
		* @brief Constructor for FAlphaBranch.
		*
		* Initializes a new FAlphaBranch instance with the given fractional and integer noise components.
		* The branch may also be linked to a previous stage (pre) for recursive noise generation.
		*
		* @param ffrac Fractional part of the noise generation (e.g., 0.5 for 1/f noise).
		* @param fint Integer part of the noise generation (e.g., 1 for integration).
		* @param pre Previous stage of the FAlphaBranch for recursive noise processing.
		* @param last Specifies if this is the last branch in the chain of processing.
		*/
		FAlphaBranch(RealType ffrac, unsigned fint, std::unique_ptr<FAlphaBranch> pre, bool last) noexcept;

		/// Default destructor.
		~FAlphaBranch() = default;

		/// Delete copy constructor to prevent copying of the FAlphaBranch.
		FAlphaBranch(const FAlphaBranch&) = delete;

		/// Delete copy assignment operator to prevent copying of the FAlphaBranch.
		FAlphaBranch& operator=(const FAlphaBranch&) = delete;

		/**
		* @brief Retrieves the current noise sample.
		*
		* This method returns the current noise sample, either by retrieving it from the buffer or by
		* calculating a new sample if this is the final stage in the noise processing.
		*
		* @return The current noise sample.
		*/
		RealType getSample() noexcept;

		/**
		* @brief Flushes the branch with a new scaling factor.
		*
		* This method reinitializes the noise generation process and applies a new scale to the previous
		* stage's noise sample.
		*
		* @param scale New scale factor to apply to the previous stage.
		*/
		void flush(RealType scale) noexcept;

		/**
		* @brief Retrieves the previous branch in the chain.
		*
		* @return Pointer to the previous FAlphaBranch, or nullptr if none exists.
		*/
		[[nodiscard]] FAlphaBranch* getPre() const noexcept { return _pre.get(); }

	private:
		/// Initializes the filters and sets up the initial state of the noise generator.
		void init();

		/// Refills the sample buffer with new upsampled noise values.
		void refill() noexcept;

		/// Calculates a new noise sample.
		RealType calcSample() noexcept;

		/**
		* @brief Filter used for shaping the noise signal.
		*/
		std::unique_ptr<signal::IirFilter> _shape_filter;

		/**
		* @brief Filter used for integrating the noise signal.
		*/
		std::unique_ptr<signal::IirFilter> _integ_filter;

		/**
		* @brief High-pass filter to remove low-frequency components from the noise.
		*/
		std::unique_ptr<signal::IirFilter> _highpass;

		/**
		* @brief Upsampler for generating higher-frequency noise components.
		*/
		std::unique_ptr<signal::DecadeUpsampler> _upsampler;

		/**
		* @brief Previous FAlphaBranch in the chain for recursive noise processing.
		*/
		std::unique_ptr<FAlphaBranch> _pre;

		/// Gain factor for shaping filter.
		RealType _shape_gain{1.0};

		/// Gain factor for integration filter.
		RealType _integ_gain{1.0};

		/// Scaling factor for the upsampled noise.
		RealType _upsample_scale{};

		/// Buffer for storing upsampled noise samples.
		std::vector<RealType> _buffer{10}; // Initializing buffer to size 10

		/// Number of samples currently in the buffer.
		unsigned _buffer_samples{};

		/// Fractional part of the noise generation.
		RealType _ffrac;

		/// Integer part of the noise generation.
		RealType _fint;

		/// Offset applied to the final noise sample.
		RealType _offset_sample{};

		/// Flag indicating if the offset sample has been retrieved.
		bool _got_offset{false};

		/// Scale factor for the previous stage's noise output.
		RealType _pre_scale{1.0};

		/// Indicates if this is the last branch in the noise processing chain.
		bool _last;
	};
}
