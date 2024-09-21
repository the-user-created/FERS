// dsp_filters.h
// Digital Signal Processing support functions
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 30 July 2007

#ifndef DSP_FILTERS_H
#define DSP_FILTERS_H

#include <memory>
#include <vector>

#include "config.h"

namespace signal
{
	void upsample(const ComplexType* in, unsigned size, ComplexType* out, unsigned ratio);

	void downsample(const std::vector<ComplexType>& in, unsigned size, std::vector<ComplexType>& out, unsigned ratio);

	class DspFilter
	{
	public:
		DspFilter() = default;

		virtual ~DspFilter() = default;

		virtual RealType filter(RealType sample) = 0;

		virtual void filter(std::vector<RealType>& samples, int size) = 0;

		// Disable copy constructor and copy assignment operator
		DspFilter(const DspFilter&) = delete;

		DspFilter& operator=(const DspFilter&) = delete;
	};

	class IirFilter final : public DspFilter
	{
	public:
		IirFilter(const std::vector<RealType>& denCoeffs, const std::vector<RealType>& numCoeffs);

		IirFilter(const RealType* denCoeffs, const RealType* numCoeffs, unsigned order);

		~IirFilter() override = default;

		RealType filter(RealType sample) override;

		void filter(std::vector<RealType>& samples, int size) override;

	private:
		std::vector<RealType> _a; // Denominator coefficients
		std::vector<RealType> _b; // Numerator coefficients
		std::vector<RealType> _w; // State (internal delay line)
		unsigned _order;
	};

	class FirFilter final : public DspFilter
	{
	public:
		// Constructor accepting a std::vector of coefficients
		explicit FirFilter(const std::vector<RealType>& coeffs) : _filter(coeffs), _w(coeffs.size(), 0),
		                                                          _order(coeffs.size()) {}

		~FirFilter() override = default;

		// Single sample filter (still not used)
		RealType filter(RealType sample) override { return 0; }

		// Filter an array of samples (still not used)
		void filter(std::vector<RealType>& samples, int size) override;

		// Filter for complex samples
		void filter(std::vector<ComplexType>& samples, unsigned size) const;

	private:
		std::vector<RealType> _filter; // FIR filter coefficients
		std::vector<RealType> _w; // State for the filter
		unsigned _order;
	};

	class ArFilter final : public DspFilter
	{
	public:
		// Note: This function is not used in the codebase
		explicit ArFilter(const std::vector<RealType>& coeffs) : _filter(coeffs), _order(coeffs.size())
		{
			_w.resize(_order, 0.0f); // Initialize _w after _order is set
		}

		~ArFilter() override = default;

		// Note: This function is not used in the codebase
		RealType filter(RealType sample) override;

		// Note: This function is not used in the codebase
		void filter(std::vector<RealType>& samples, int size) override;

	private:
		std::vector<RealType> _w; // Store internal state
		std::vector<RealType> _filter; // Coefficients
		unsigned _order;
	};

	class Upsampler
	{
	public:
		// Note: This function is not used in the codebase
		explicit Upsampler(int ratio);

		~Upsampler() = default;

		// Note: This function is not used in the codebase
		void upsample(const RealType* inSamples, int inSize, RealType* outSamples, int outSize);

		// Disable copy constructor and copy assignment operator
		Upsampler(const Upsampler&) = delete;

		Upsampler& operator=(const Upsampler&) = delete;

	private:
		int _ratio;
		std::vector<RealType> _filterbank;
		std::vector<RealType> _sample_memory;
		int _filter_size;

		RealType getSample(const RealType* samples, const int n) const
		{
			return n >= 0 ? samples[n] : _sample_memory[n + _filter_size];
		}
	};

	class DecadeUpsampler
	{
	public:
		DecadeUpsampler();

		~DecadeUpsampler() = default;

		void upsample(RealType sample, std::vector<RealType>& out) const;

		// Note: This function is not used in the codebase
		void upsample(const std::vector<RealType>& in, int count, std::vector<RealType>& out) const;

		// Disable copy constructor and copy assignment operator
		DecadeUpsampler(const DecadeUpsampler&) = delete;

		DecadeUpsampler& operator=(const DecadeUpsampler&) = delete;

	private:
		std::unique_ptr<IirFilter> _filter;
	};
}

#endif
