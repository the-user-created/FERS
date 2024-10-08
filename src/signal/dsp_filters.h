/**
* @file dsp_filters.h
* @brief Header file for Digital Signal Processing (DSP) filters and upsampling/downsampling functionality.
*
* This file contains declarations for several DSP filter classes and functions. These include IIR, FIR, and AR
* filters, as well as upsampling and downsampling operations using these filters. The classes and functions
* are designed to support efficient real-time DSP operations, including sample-based and block-based filtering.
*
* The filters are designed to work with both real and complex data types and provide a flexible API for
* integration into signal processing pipelines.
*
* @authors David Young, Marc Brooker
* @date 2007-07-30
*/

#pragma once

#include <memory>
#include <span>
#include <vector>

#include "config.h"

namespace signal
{
	/**
	* @brief Upsamples a signal by a given ratio.
	*
	* Takes an input signal and upscales it by inserting zeros between the samples and applying a filter
	* to reconstruct the signal at a higher sample rate.
	*
	* @param in Input span of complex samples.
	* @param out Output span for upsampled complex samples.
	* @param ratio Upsampling ratio.
	* @throws std::invalid_argument if the input or output spans are empty or the ratio is zero.
	*/
	void upsample(std::span<const ComplexType> in, std::span<ComplexType> out, unsigned ratio);

	/**
	* @brief Downsamples a signal by a given ratio.
	*
	* Takes an input signal and downsamples it by filtering and then selecting samples at intervals determined by the ratio.
	*
	* @param in Input span of complex samples.
	* @param out Output span for downsampled complex samples.
	* @param ratio Downsampling ratio.
	* @throws std::invalid_argument if the input or output spans are empty or the ratio is zero.
	*/
	void downsample(std::span<const ComplexType> in, std::span<ComplexType> out, unsigned ratio);

	/**
	* @class DspFilter
	* @brief Abstract base class for digital filters.
	*
	* This class serves as a base class for various digital filters, providing common interface functions for filtering.
	* Derived classes must implement the sample-based and block-based filtering functions.
	*/
	class DspFilter
	{
	public:
		DspFilter() = default;

		virtual ~DspFilter() = default;

		/**
		* @brief Filters a single sample.
		*
		* @param sample A single real-valued sample to be filtered.
		* @return The filtered sample.
		*/
		virtual RealType filter(RealType sample) = 0;

		/**
	    * @brief Filters a block of samples.
	    *
	    * @param samples Span of real-valued samples to be filtered.
	    */
		virtual void filter(std::span<RealType> samples) = 0;

		// Disable copy constructor and copy assignment operator
		DspFilter(const DspFilter&) = delete;

		DspFilter& operator=(const DspFilter&) = delete;

		// Enable default move semantics
		DspFilter(DspFilter&&) noexcept = default;

		DspFilter& operator=(DspFilter&&) noexcept = default;
	};

	/**
	* @class IirFilter
	* @brief Implements an Infinite Impulse Response (IIR) filter.
	*
	* The IIR filter is characterized by a set of numerator and denominator coefficients. It provides methods
	* for filtering single samples or blocks of samples. The filter is designed for real-valued signals.
	*/
	class IirFilter final : public DspFilter
	{
	public:
		/**
		* @brief Constructs an IIR filter with given numerator and denominator coefficients.
		*
		* @param denCoeffs Span of denominator coefficients.
		* @param numCoeffs Span of numerator coefficients.
		* @throws std::logic_error if the coefficient sizes do not match.
		*/
		IirFilter(std::span<const RealType> denCoeffs, std::span<const RealType> numCoeffs);

		/**
		* @brief Constructs an IIR filter with given numerator and denominator coefficients and order.
		*
		* @param denCoeffs Pointer to the denominator coefficients array.
		* @param numCoeffs Pointer to the numerator coefficients array.
		* @param order The order of the filter.
		*/
		IirFilter(const RealType* denCoeffs, const RealType* numCoeffs, unsigned order) noexcept;

		~IirFilter() override = default;

		/**
		 * @brief Filters a single sample.
		 *
		 * @param sample The sample to be filtered.
		 * @return The filtered sample.
		 */
		RealType filter(RealType sample) noexcept override;

		/**
		 * @brief Filters a block of samples.
		 *
		 * @param samples Span of samples to be filtered.
		 */
		void filter(std::span<RealType> samples) noexcept override;

	private:
		std::vector<RealType> _a; ///< Denominator coefficients
		std::vector<RealType> _b; ///< Numerator coefficients
		std::vector<RealType> _w; ///< Internal state
		unsigned _order{}; ///< Filter order
	};

	/**
	* @class FirFilter
	* @brief Implements a Finite Impulse Response (FIR) filter.
	*
	* The FIR filter is characterized by a set of coefficients and is used for real and complex-valued signal processing.
	*/
	class FirFilter final : public DspFilter
	{
	public:
		/**
		* @brief Constructs an FIR filter with the given coefficients.
		*
		* @param coeffs Span of filter coefficients.
		*/
		explicit FirFilter(std::span<const RealType> coeffs)
			: _filter(coeffs.begin(), coeffs.end()), _w(coeffs.size()), _order(coeffs.size()) {}

		~FirFilter() override = default;

		RealType filter(RealType) override { return 0; } // Not used

		/**
		 * @brief Filters a block of samples.
		 *
		 * @param samples Span of samples to be filtered.
		 */
		void filter(std::span<RealType> samples) noexcept override;

		/**
		 * @brief Filters a block of complex samples.
		 *
		 * @param samples Span of complex samples to be filtered.
		 */
		void filter(std::span<ComplexType> samples) const noexcept;

	private:
		std::vector<RealType> _filter; ///< Filter coefficients
		std::vector<RealType> _w; ///< Internal state
		unsigned _order{}; ///< Filter order
	};

	/**
	* @class ArFilter
	* @brief Implements an Autoregressive (AR) filter.
	*
	* The AR filter applies autoregressive filtering to real-valued signals based on the provided coefficients.
	*/
	class ArFilter final : public DspFilter
	{
	public:
		/**
		* @brief Constructs an AR filter with the given coefficients.
		*
		* @param coeffs Span of AR filter coefficients.
		*/
		explicit ArFilter(std::span<const RealType> coeffs) noexcept
			: _w(coeffs.size()), _filter(coeffs.begin(), coeffs.end()), _order(coeffs.size()) {}

		~ArFilter() override = default;

		/**
		 * @brief Filters a single sample.
		 *
		 * @param sample The sample to be filtered.
		 * @return The filtered sample.
		 */
		RealType filter(RealType sample) noexcept override;

		/**
		 * @brief Filters a block of samples.
		 *
		 * @param samples Span of samples to be filtered.
		 */
		void filter(std::span<RealType> samples) noexcept override;

	private:
		std::vector<RealType> _w; ///< Internal state
		std::vector<RealType> _filter; ///< Filter coefficients
		unsigned _order{}; ///< Filter order

		/**
		 * @brief Applies the AR filter to a single sample.
		 *
		 * @param sample The sample to be filtered.
		 * @return The filtered sample.
		 */
		RealType applyFilter(RealType sample) noexcept;
	};

	/**
	* @class Upsampler
	* @brief Implements an upsampler for increasing the sample rate of a signal.
	*
	* The upsampler increases the sample rate by an integer ratio using polyphase filtering.
	*/
	class Upsampler
	{
	public:
		/**
		* @brief Constructs an upsampler with the specified ratio.
		*
		* @param ratio Upsampling ratio.
		*/
		explicit Upsampler(int ratio) noexcept;

		~Upsampler() = default;

		/**
		* @brief Upsamples a block of input samples.
		*
		* @param inSamples Span of input samples.
		* @param outSamples Span of output samples.
		* @throws std::runtime_error if the output span size does not match the expected upsampled size.
		*/
		void upsample(std::span<const RealType> inSamples, std::span<RealType> outSamples);

		// Disable copy constructor and copy assignment operator
		Upsampler(const Upsampler&) = delete;

		Upsampler& operator=(const Upsampler&) = delete;

		// Enable default move semantics
		Upsampler(Upsampler&&) noexcept = default;

		Upsampler& operator=(Upsampler&&) noexcept = default;

	private:
		int _ratio; ///< Upsampling ratio.
		int _filter_size; ///< Size of the filter.
		std::vector<RealType> _filterbank; ///< Coefficients of the polyphase filter.
		std::vector<RealType> _sample_memory; ///< Memory for previous samples (used in filtering).

		/**
		* @brief Retrieves a sample from input or memory buffer.
		*
		* @param samples Span of input samples.
		* @param n Index to retrieve.
		* @return The retrieved sample.
		*/
		[[nodiscard]] RealType getSample(const std::span<const RealType> samples, const long n) const
		{
			return n >= 0 ? samples[n] : _sample_memory[n + _filter_size];
		}
	};

	/**
	 * @class DecadeUpsampler
	 * @brief Implements a specialized upsampler with a fixed upsampling factor of 10.
	 *
	 * This class uses an 11th order elliptic lowpass filter for upsampling by a factor of 10.
	 */
	class DecadeUpsampler
	{
	public:
		DecadeUpsampler();

		~DecadeUpsampler() = default;

		/**
		 * @brief Upsamples a single sample.
		 *
		 * @param sample The sample to be upsampled.
		 * @param out Span of output samples.
		 */
		void upsample(RealType sample, std::span<RealType> out) const;

		/**
		 * @brief Upsamples a block of samples.
		 *
		 * @param in Span of input samples.
		 * @param out Span of output samples.
		 */
		void upsample(std::span<const RealType> in, std::span<RealType> out) const;

		// Disable copy constructor and copy assignment operator
		DecadeUpsampler(const DecadeUpsampler&) = delete;

		DecadeUpsampler& operator=(const DecadeUpsampler&) = delete;

		// Enable default move semantics
		DecadeUpsampler(DecadeUpsampler&&) noexcept = default;

		DecadeUpsampler& operator=(DecadeUpsampler&&) noexcept = default;

	private:
		std::unique_ptr<IirFilter> _filter; ///< IIR filter for upsampling
	};
}
