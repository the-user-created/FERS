//Digital Signal Processing support functions
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//30 July 2007

#ifndef RS_DSP_H
#define RS_DSP_H

#include <complex>
#include <config.h>
#include <vector>
#include <boost/utility.hpp>

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
		virtual rsFloat filter(rsFloat sample) = 0;

		/// Pass an array of samples through the filter, filtering in place
		virtual void filter(rsFloat* samples, int size) = 0;
	};

	/// IIR (ARMA) Digital Filter, implemented with Direct Form II
	// Supports filters of the type A(z)/B(z)
	class IirFilter final : public DspFilter
	{
	public:
		/// Constructor
		IirFilter(const std::vector<rsFloat>& denCoeffs, const std::vector<rsFloat>& numCoeffs);

		/// Constuctor
		IirFilter(const rsFloat* denCoeffs, const rsFloat* numCoeffs, unsigned int order);

		/// Destructor
		virtual ~IirFilter();

		/// Pass a single sample through the filter
		virtual rsFloat filter(rsFloat sample);

		/// Pass an array of samples through the filter, filtering in place
		virtual void filter(rsFloat* samples, int size);

	private:
		rsFloat* _w; //!< Past x values
		// rsFloat *wy; //!< Past y values
		rsFloat* _a; //!< Denominator co-efficients
		rsFloat* _b; //!< Numerator co-efficients
		unsigned int _order; //!< Filter order
	};

	/// FIR (MA) Digital Filter
	//Supports filters of the type B(z)/1
	class FirFilter final : public DspFilter
	{
	public:
		/// Constructor
		explicit FirFilter(const std::vector<rsFloat>& coeffs);

		FirFilter(const rsFloat* coeffs, int count);

		/// Destructor
		virtual ~FirFilter();

		/// Pass a single sample through the filter
		virtual rsFloat filter(rsFloat sample);

		/// Pass an array of samples through the filter, filtering in place
		virtual void filter(rsFloat* samples, int size);

		/// Pass an array of complex samples through the filter, filtering in place
		void filter(std::complex<rsFloat>* samples, int size) const;

	private:
		rsFloat* _w; //!< Filter state
		rsFloat* _filter; //!< Filter coefficients
		unsigned int _order; //!< Filter order
	};

	/// Auto Regressive (AR) Digital Filter
	//Supports filters of the type 1/A(z)
	class ArFilter final : public DspFilter
	{
	public:
		/// Constructor
		explicit ArFilter(const std::vector<rsFloat>& coeffs);

		/// Destructor
		~ArFilter();

		/// Pass a single sample through the filter
		virtual rsFloat filter(rsFloat sample);

		/// Pass an array of samples through the filter, filtering in place
		virtual void filter(rsFloat* samples, int size);

	private:
		rsFloat* _w; //!< Filter state
		rsFloat* _filter; //!< Filter coefficients
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
		void upsample(const rsFloat* inSamples, int inSize, rsFloat* outSamples, int outSize) const;

	private:
		int _ratio; //!< Upsampling ratio
		rsFloat* _filterbank; //!< FIR polyphase filter bank
		rsFloat* _sample_memory; //!< Last samples used, to allow seamless upsampling in blocks
		int _filter_size; //!< Length of the interpolation filter
		//Get a sample, from either the provided pointer or sample memory
		inline rsFloat getSample(const rsFloat* samples, int n) const;
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
		void upsample(rsFloat sample, rsFloat* out) const;

		/// Upsample a large block, out must be ten times bigger than in
		void upsample(const rsFloat* in, int count, rsFloat* out) const;

	private:
		/// Anti-imaging filter
		IirFilter* _filter;
	};
};

#endif
