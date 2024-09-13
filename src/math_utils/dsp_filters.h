// dsp_filters.h
// Digital Signal Processing support functions
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 30 July 2007

#ifndef DSP_FILTERS_H
#define DSP_FILTERS_H

#include <complex>
#include <vector>
#include <boost/utility.hpp>

#include "config.h"

namespace rs
{
	void upsample(const RS_COMPLEX* in, unsigned size, RS_COMPLEX* out, unsigned ratio);

	void downsample(const RS_COMPLEX* in, unsigned size, RS_COMPLEX* out, unsigned ratio);

	class DspFilter : boost::noncopyable
	{
	public:
		DspFilter() = default;

		virtual ~DspFilter() = default;

		virtual RS_FLOAT filter(RS_FLOAT sample) = 0;

		virtual void filter(RS_FLOAT* samples, int size) = 0;
	};

	class IirFilter final : public DspFilter
	{
	public:
		IirFilter(const std::vector<RS_FLOAT>& denCoeffs, const std::vector<RS_FLOAT>& numCoeffs);

		IirFilter(const RS_FLOAT* denCoeffs, const RS_FLOAT* numCoeffs, unsigned order);

		~IirFilter() override;

		RS_FLOAT filter(RS_FLOAT sample) override;

		void filter(RS_FLOAT* samples, int size) override;

	private:
		RS_FLOAT* _w;
		RS_FLOAT* _a;
		RS_FLOAT* _b;
		unsigned _order;
	};

	class FirFilter final : public DspFilter
	{
	public:
		explicit FirFilter(const std::vector<RS_FLOAT>& coeffs);

		FirFilter(const RS_FLOAT* coeffs, unsigned count);

		~FirFilter() override;

		RS_FLOAT filter(RS_FLOAT sample) override;

		void filter(RS_FLOAT* samples, int size) override;

		void filter(std::complex<RS_FLOAT>* samples, unsigned size) const;

	private:
		RS_FLOAT* _w;
		RS_FLOAT* _filter;
		unsigned _order;
	}; // TODO: FirFilter class is not used, consider removing it

	class ArFilter final : public DspFilter
	{
	public:
		explicit ArFilter(const std::vector<RS_FLOAT>& coeffs);

		~ArFilter() override;

		RS_FLOAT filter(RS_FLOAT sample) override;

		void filter(RS_FLOAT* samples, int size) override;

	private:
		RS_FLOAT* _w;
		RS_FLOAT* _filter;
		unsigned _order;
	}; // TODO: ArFilter class is not used, consider removing it

	class Upsampler : boost::noncopyable
	{
	public:
		explicit Upsampler(int ratio);

		~Upsampler();

		void upsample(const RS_FLOAT* inSamples, int inSize, RS_FLOAT* outSamples, int outSize) const;

	private:
		int _ratio;
		RS_FLOAT* _filterbank;
		RS_FLOAT* _sample_memory;
		int _filter_size;

		inline RS_FLOAT getSample(const RS_FLOAT* samples, int n) const;
	};

	class DecadeUpsampler : boost::noncopyable
	{
	public:
		DecadeUpsampler();

		~DecadeUpsampler();

		void upsample(RS_FLOAT sample, RS_FLOAT* out) const;

		void upsample(const RS_FLOAT* in, int count, RS_FLOAT* out) const;

	private:
		IirFilter* _filter;
	};
}

#endif
