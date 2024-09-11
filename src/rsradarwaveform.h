//rsradarwaveform.h
//Classes for different types of radar waveforms
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//8 June 2006

#ifndef RS_RADARWAVEFORM_H
#define RS_RADARWAVEFORM_H

#include <complex>
#include <string>
#include <vector>
#include <boost/shared_array.hpp>
#include <boost/utility.hpp>

#include "config.h"
#include "rspolarize.h"

namespace rs
{
	///Forward Declarations
	class Signal; //rssignal.h
	class JonesVector; //rspolarize.h

	///Complex type for rendering operations
	typedef std::complex<RS_FLOAT> RsComplex;

	/// A continuous wave response interpolation point
	struct InterpPoint
	{
		/// Constructor
		InterpPoint(RS_FLOAT power, RS_FLOAT start, RS_FLOAT delay, RS_FLOAT doppler, RS_FLOAT phase,
		            RS_FLOAT noiseTemperature);

		RS_FLOAT power; //!< Peak power of the pulse (into 1ohm)
		RS_FLOAT time; //!< Start time (seconds)
		RS_FLOAT delay; //!< Pulse round trip time (seconds)
		RS_FLOAT doppler; //!< Doppler shift (radians)
		RS_FLOAT phase; //!< Phase (radians)
		RS_FLOAT noise_temperature; //!< Noise temperature (kelvin)
	};

	/// The RadarWaveform class stores information about a pulse shape (or continuous wave wave)
	class RadarSignal : public boost::noncopyable
	{
	public:
		/// Default constructor
		RadarSignal(std::string  name, RS_FLOAT power, RS_FLOAT carrierfreq, RS_FLOAT length, Signal* signal);

		/// Destructor
		~RadarSignal();

		/// Get the signal power
		[[nodiscard]] RS_FLOAT getPower() const;

		/// Get the signal carrier frequency (Hz)
		[[nodiscard]] RS_FLOAT getCarrier() const;

		/// Get the name of the signal
		[[nodiscard]] std::string getName() const;

		/// Return the native sample rate of the waveform
		[[nodiscard]] RS_FLOAT getRate() const;

		/// Return the length of the pulse
		[[nodiscard]] RS_FLOAT getLength() const;

		/// Render the pulse with the given parameters
		boost::shared_array<RsComplex> render(const std::vector<InterpPoint>& points, unsigned int& size,
		                                      RS_FLOAT fracWinDelay) const;

		/// Get the signal polarization
		JonesVector getPolarization();

		/// Set the signal polarization
		void setPolarization(const JonesVector& in);

	private:
		std::string _name; //!< The name of the pulse
		RS_FLOAT _power; //!< Power of the signal transmitted (Watts in 1ohm)
		RS_FLOAT _carrierfreq; //!< Carrier frequency (Hz)
		RS_FLOAT _length; //!< Length of the signal (seconds)
		Signal* _signal; //!< Transmitted Signal
		JonesVector _polar; //!< Signal Polarization
	};
}

//Functions for creating radar waveforms
namespace rs_pulse_factory
{
	/// Load a pulse from a file
	rs::RadarSignal* loadPulseFromFile(const std::string& name, const std::string& filename, RS_FLOAT power,
	                                   RS_FLOAT carrierFreq);
}


#endif
