//signal.cpp
//Interface for the signal class
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started 24 May 2006

#ifndef RS_SIGNAL_H
#define RS_SIGNAL_H

#include <complex>
#include <config.h>
#include <boost/shared_array.hpp>
#include <boost/utility.hpp>

#include "rsradarwaveform.h"

///Forward declarations
namespace rs
{
	class Signal; //rssignal.h
}

namespace rs_signal
{
	/// Type for storing signal
	typedef std::complex<rsFloat> Complex;

	/// Add noise to the signal with the given temperature
	void addNoise(rsFloat* data, rsFloat temperature, unsigned int size, rsFloat fs);

	/// Demodulate a frequency domain signal into time domain I and Q
	Complex* iqDemodulate(rsFloat* data, unsigned int size, rsFloat phase);

	/// Simulate the effect of and ADC converter on the signal
	void adcSimulate(Complex* data, unsigned int size, int bits, rsFloat fullscale);
}

namespace rs
{
	/// Class to store and process a time domain signal
	class Signal : boost::noncopyable
	{
	public:
		Signal(); //!< Default constructor
		~Signal(); //!< Default destructor

		/// Clear deletes the data currently associated with the signal
		void clear();

		/// Load data into the signal (time domain, complex)
		void load(const rs_signal::Complex* inData, unsigned int samples, rsFloat sampleRate);

		/// Load data into the signal (time domain, real)
		void load(const rsFloat* inData, unsigned int samples, rsFloat sampleRate);

		/// Return the sample rate of the signal
		rsFloat rate() const;

		/// Return the size, in samples of the signal
		unsigned int size() const;

		/// Get a copy of the signal domain data
		rsFloat* copyData() const;

		/// Get the number of samples of padding at the beginning of the pulse
		unsigned int pad() const;

		/// Render the pulse with the specified doppler, delay and amplitude
		boost::shared_array<rs::RsComplex> render(const std::vector<InterpPoint>& points, rsFloat transPower,
		                                          unsigned int& size, rsFloat fracWinDelay) const;

	private:
		/// The signal data
		rs_signal::Complex* _data;
		/// Size of the signal in samples
		unsigned int _size;
		/// The sample rate of the signal in the time domain
		rsFloat _rate;
		/// Number of samples of padding at the beginning of the pulse
		unsigned int _pad;
	};
}
#endif
