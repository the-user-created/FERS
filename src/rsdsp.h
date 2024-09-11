//Digital Signal Processing support functions
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//30 July 2007

#ifndef RS_DSP_H
#define RS_DSP_H

#include <complex>
#include <vector>
#include <boost/utility.hpp>

#include "config.h"
#include "rsradarwaveform.h"

namespace rs
{
	//
	// Support functions
	//

	/// Upsample size samples stored *in by an integer ratio and store the result in (pre-allocated) out
	void upsample(const RsComplex* in, int size, RsComplex* out, int ratio);

	/// Downsample size samples stored *in by an integer ratio and store the result in (pre-allocated) out
	void downsample(const RsComplex* in, int size, RsComplex* out, int ratio);

	/// Filter, parent class for digital filters
	class DspFilter : boost::noncopyable
	{
	public:
		/// Constructor
		DspFilter();

		/// Destructor
		virtual ~DspFilter();

		/// Pass a single sample through the filter
		virtual RS_FLOAT filter(RS_FLOAT sample) = 0;

		/// Pass an array of samples through the filter, filtering in place
		virtual void filter(RS_FLOAT* samples, int size) = 0;
	};

	/// IIR (ARMA) Digital Filter, implemented with Direct Form II
	// Supports filters of the type A(z)/B(z)
	class IirFilter final : public DspFilter
	{
	public:
		/// Constructor
		IirFilter(const std::vector<RS_FLOAT>& denCoeffs, const std::vector<RS_FLOAT>& numCoeffs);

		/// Constuctor
		IirFilter(const RS_FLOAT* denCoeffs, const RS_FLOAT* numCoeffs, unsigned int order);

		/// Destructor
		~IirFilter() override;

		/// Pass a single sample through the filter
		RS_FLOAT filter(RS_FLOAT sample) override;

		/// Pass an array of samples through the filter, filtering in place
		void filter(RS_FLOAT* samples, int size) override;

	private:
		RS_FLOAT* _w; //!< Past x values
		// rsFloat *wy; //!< Past y values
		RS_FLOAT* _a; //!< Denominator co-efficients
		RS_FLOAT* _b; //!< Numerator co-efficients
		unsigned int _order; //!< Filter order
	};

	/// FIR (MA) Digital Filter
	//Supports filters of the type B(z)/1
	class FirFilter final : public DspFilter
	{
	public:
		/// Constructor
		explicit FirFilter(const std::vector<RS_FLOAT>& coeffs);

		FirFilter(const RS_FLOAT* coeffs, int count);

		/// Destructor
		~FirFilter() override;

		/// Pass a single sample through the filter
		RS_FLOAT filter(RS_FLOAT sample) override;

		/// Pass an array of samples through the filter, filtering in place
		void filter(RS_FLOAT* samples, int size) override;

		/// Pass an array of complex samples through the filter, filtering in place
		void filter(std::complex<RS_FLOAT>* samples, int size) const;

	private:
		RS_FLOAT* _w; //!< Filter state
		RS_FLOAT* _filter; //!< Filter coefficients
		unsigned int _order; //!< Filter order
	};

	/// Auto Regressive (AR) Digital Filter
	//Supports filters of the type 1/A(z)
	class ArFilter final : public DspFilter
	{
	public:
		/// Constructor
		explicit ArFilter(const std::vector<RS_FLOAT>& coeffs);

		/// Destructor
		~ArFilter() override;

		/// Pass a single sample through the filter
		RS_FLOAT filter(RS_FLOAT sample) override;

		/// Pass an array of samples through the filter, filtering in place
		void filter(RS_FLOAT* samples, int size) override;

	private:
		RS_FLOAT* _w; //!< Filter state
		RS_FLOAT* _filter; //!< Filter coefficients
		unsigned int _order; //!< Filter order
	};

	/// Upsamples a signal and applies an anti-imaging filter
	// Implemented using polyphase FIR filter with windowed sinc
	class Upsampler : boost::noncopyable
	{
	public:
		/// Constructor (ratio of upsampling, co-efficients of anti-imaging filter)
		explicit Upsampler(int ratio);

		/// Destructor
		~Upsampler();

		/// Upsample the given sample to a pre-allocated target
		void upsample(const RS_FLOAT* inSamples, int inSize, RS_FLOAT* outSamples, int outSize) const;

	private:
		int _ratio; //!< Upsampling ratio
		RS_FLOAT* _filterbank; //!< FIR polyphase filter bank
		RS_FLOAT* _sample_memory; //!< Last samples used, to allow seamless upsampling in blocks
		int _filter_size; //!< Length of the interpolation filter
		//Get a sample, from either the provided pointer or sample memory
		inline RS_FLOAT getSample(const RS_FLOAT* samples, int n) const;
	};

	/// Upsample a signal by a factor of 10
	class DecadeUpsampler : boost::noncopyable
	{
	public:
		/// Constructor
		DecadeUpsampler();

		/// Destrubtor
		~DecadeUpsampler();

		/// Upsample one sample at a time, out is array of ten samples
		void upsample(RS_FLOAT sample, RS_FLOAT* out) const;

		/// Upsample a large block, out must be ten times bigger than in
		void upsample(const RS_FLOAT* in, int count, RS_FLOAT* out) const;

	private:
		/// Anti-imaging filter
		IirFilter* _filter;
	};
}

#endif
