//rsradarwaveform.h
//Classes for different types of radar waveforms
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//8 June 2006

#ifndef RS_RADARWAVEFORM_H
#define RS_RADARWAVEFORM_H

#include <complex>
#include <config.h>
#include <string>
#include <vector>
#include <boost/shared_array.hpp>
#include <boost/utility.hpp>

#include "rspolarize.h"

namespace rs
{
	///Forward Declarations
	class Signal; //rssignal.h
	class JonesVector; //rspolarize.h

	///Complex type for rendering operations
	typedef std::complex<rsFloat> RsComplex;

	/// A continuous wave response interpolation point
	struct InterpPoint
	{
		/// Constructor
		InterpPoint(rsFloat power, rsFloat start, rsFloat delay, rsFloat doppler, rsFloat phase,
		            rsFloat noiseTemperature);

		rsFloat power; //!< Peak power of the pulse (into 1ohm)
		rsFloat time; //!< Start time (seconds)
		rsFloat delay; //!< Pulse round trip time (seconds)
		rsFloat doppler; //!< Doppler shift (radians)
		rsFloat phase; //!< Phase (radians)
		rsFloat noise_temperature; //!< Noise temperature (kelvin)
	};

	/// The RadarWaveform class stores information about a pulse shape (or continuous wave wave)
	class RadarSignal : public boost::noncopyable
	{
	public:
		/// Default constructor
		RadarSignal(const std::string& name, rsFloat power, rsFloat carrierfreq, rsFloat length, Signal* signal);

		/// Destructor
		~RadarSignal();

		/// Get the signal power
		rsFloat getPower() const;

		/// Get the signal carrier frequency (Hz)
		rsFloat getCarrier() const;

		/// Get the name of the signal
		std::string getName() const;

		/// Return the native sample rate of the waveform
		rsFloat getRate() const;

		/// Return the length of the pulse
		rsFloat getLength() const;

		/// Render the pulse with the given parameters
		boost::shared_array<RsComplex> render(const std::vector<InterpPoint>& points, unsigned int& size,
		                                      rsFloat fracWinDelay) const;

		/// Get the signal polarization
		JonesVector getPolarization();

		/// Set the signal polarization
		void setPolarization(const JonesVector& in);

	private:
		std::string _name; //!< The name of the pulse
		rsFloat _power; //!< Power of the signal transmitted (Watts in 1ohm)
		rsFloat _carrierfreq; //!< Carrier frequency (Hz)
		rsFloat _length; //!< Length of the signal (seconds)
		Signal* _signal; //!< Transmitted Signal
		JonesVector _polar; //!< Signal Polarization
	};
}

//Functions for creating radar waveforms
namespace rs_pulse_factory
{
	/// Load a pulse from a file
	rs::RadarSignal* loadPulseFromFile(const std::string& name, const std::string& filename, rsFloat power,
	                                   rsFloat carrierFreq);
}


#endif
