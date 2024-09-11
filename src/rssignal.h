//signal.cpp
//Interface for the signal class
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started 24 May 2006

#ifndef RS_SIGNAL_H
#define RS_SIGNAL_H

#include <complex>
#include <boost/shared_array.hpp>
#include <boost/utility.hpp>

#include "config.h"
#include "rsradarwaveform.h"

///Forward declarations
namespace rs
{
	class Signal; //rssignal.h
}

namespace rs_signal
{
	/// Type for storing signal
	using Complex = std::complex<RS_FLOAT>;

	/// Add noise to the signal with the given temperature
	void addNoise(RS_FLOAT* data, RS_FLOAT temperature, unsigned int size, RS_FLOAT fs);

	/// Demodulate a frequency domain signal into time domain I and Q
	Complex* iqDemodulate(RS_FLOAT* data, unsigned int size, RS_FLOAT phase);

	/// Simulate the effect of and ADC converter on the signal
	void adcSimulate(Complex* data, unsigned int size, int bits, RS_FLOAT fullscale);
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
		void load(const rs_signal::Complex* inData, unsigned int samples, RS_FLOAT sampleRate);

		/// Load data into the signal (time domain, real)
		void load(const RS_FLOAT* inData, unsigned int samples, RS_FLOAT sampleRate);

		/// Return the sample rate of the signal
		[[nodiscard]] RS_FLOAT rate() const;

		/// Return the size, in samples of the signal
		[[nodiscard]] unsigned int size() const;

		/// Get a copy of the signal domain data
		[[nodiscard]] RS_FLOAT* copyData() const;

		/// Get the number of samples of padding at the beginning of the pulse
		[[nodiscard]] unsigned int pad() const; // TODO: unused

		/// Render the pulse with the specified doppler, delay and amplitude
		boost::shared_array<RsComplex> render(const std::vector<InterpPoint>& points,
		                                          unsigned int& size, double fracWinDelay) const;

	private:
		/// The signal data
		rs_signal::Complex* _data;
		/// Size of the signal in samples
		unsigned int _size;
		/// The sample rate of the signal in the time domain
		RS_FLOAT _rate;
		/// Number of samples of padding at the beginning of the pulse
		unsigned int _pad{};
	};
}
#endif
