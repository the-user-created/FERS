// dsp_filters.h
// Digital Signal Processing support functions
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 30 July 2007

#ifndef DSP_FILTERS_H
#define DSP_FILTERS_H

#include <complex>
#include <memory>
#include <vector>

#include "config.h"

namespace rs
{
	void upsample(const RS_COMPLEX* in, unsigned size, RS_COMPLEX* out, unsigned ratio);

	void downsample(const RS_COMPLEX* in, unsigned size, RS_COMPLEX* out, unsigned ratio);

	class DspFilter
	{
	public:
		DspFilter() = default;

		virtual ~DspFilter() = default;

		virtual RS_FLOAT filter(RS_FLOAT sample) = 0;

		virtual void filter(RS_FLOAT* samples, int size) = 0;

		// Disable copy constructor and copy assignment operator
		DspFilter(const DspFilter&) = delete;
		DspFilter& operator=(const DspFilter&) = delete;
	};

	class IirFilter final : public DspFilter
	{
	public:
		IirFilter(const std::vector<RS_FLOAT>& denCoeffs, const std::vector<RS_FLOAT>& numCoeffs);

		IirFilter(const RS_FLOAT* denCoeffs, const RS_FLOAT* numCoeffs, unsigned order);

		~IirFilter() override = default;

		RS_FLOAT filter(RS_FLOAT sample) override;

		void filter(RS_FLOAT* samples, int size) override;

	private:
		std::vector<RS_FLOAT> _a; // Denominator coefficients
		std::vector<RS_FLOAT> _b; // Numerator coefficients
		std::vector<RS_FLOAT> _w; // State (internal delay line)
		unsigned _order;
	};

	class FirFilter final : public DspFilter
	{
	public:
		// Constructor accepting a std::vector of coefficients
		explicit FirFilter(const std::vector<RS_FLOAT>& coeffs) : _filter(coeffs), _w(coeffs.size(), 0),
		_order(coeffs.size()) {}

		~FirFilter() override = default;

		// Single sample filter (still not used)
		RS_FLOAT filter(RS_FLOAT sample) override { return 0; }

		// Filter an array of samples (still not used)
		void filter(RS_FLOAT* samples, int size) override;

		// Filter for complex samples
		void filter(std::vector<RS_COMPLEX>& samples, unsigned size) const;

	private:
		std::vector<RS_FLOAT> _filter; // FIR filter coefficients
		std::vector<RS_FLOAT> _w; // State for the filter
		unsigned _order;
	};

	class ArFilter final : public DspFilter
	{
	public:
		// Note: This function is not used in the codebase
		explicit ArFilter(const std::vector<RS_FLOAT>& coeffs) : _filter(coeffs), _order(coeffs.size())
		{
			_w.resize(_order, 0.0f); // Initialize _w after _order is set
		}

		~ArFilter() override = default;

		// Note: This function is not used in the codebase
		RS_FLOAT filter(RS_FLOAT sample) override;

		// Note: This function is not used in the codebase
		void filter(RS_FLOAT* samples, int size) override;

	private:
		std::vector<RS_FLOAT> _w; // Store internal state
		std::vector<RS_FLOAT> _filter; // Coefficients
		unsigned _order;
	};

	class Upsampler
	{
	public:
		// Note: This function is not used in the codebase
		explicit Upsampler(int ratio);

		~Upsampler() = default;

		// Note: This function is not used in the codebase
		void upsample(const RS_FLOAT* inSamples, int inSize, RS_FLOAT* outSamples, int outSize);

		// Disable copy constructor and copy assignment operator
		Upsampler(const Upsampler&) = delete;

		Upsampler& operator=(const Upsampler&) = delete;

	private:
		int _ratio;
		std::vector<RS_FLOAT> _filterbank;
		std::vector<RS_FLOAT> _sample_memory;
		int _filter_size;

		RS_FLOAT getSample(const RS_FLOAT* samples, const int n) const
		{
			return n >= 0 ? samples[n] : _sample_memory[n + _filter_size];
		}
	};

	class DecadeUpsampler
	{
	public:
		DecadeUpsampler();

		~DecadeUpsampler() = default;

		void upsample(RS_FLOAT sample, RS_FLOAT* out) const;

		// Note: This function is not used in the codebase
		void upsample(const RS_FLOAT* in, int count, RS_FLOAT* out) const;

		// Disable copy constructor and copy assignment operator
		DecadeUpsampler(const DecadeUpsampler&) = delete;
		DecadeUpsampler& operator=(const DecadeUpsampler&) = delete;

	private:
		std::unique_ptr<IirFilter> _filter;
	};
}

#endif
