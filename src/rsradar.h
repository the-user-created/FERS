//rsradar.h
// Defines classes for receivers, transmitters and antennas
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 21 April 2006

#ifndef RS_RADAR_H
#define RS_RADAR_H

#include <string>
#include <vector>
#include <boost/thread/mutex.hpp>

#include "config.h"
#include "rsobject.h"
#include "rsradarwaveform.h"

namespace rs
{
	//Forward declaration of Response (see rsresponse.h)
	class Response;

	//Forward declaration of Antenna (see rsantenna.h)
	class Antenna;

	//Forward declaration of SVec3 and Vec3 (see rspath.h)
	class SVec3;
	class Vec3;

	//Forward declaration of Timing (see rstiming.h)
	class Timing;

	//Forward declaration of Receiver and Transmitter (this file)
	class Receiver;
	class Transmitter;

	//Forward declaration of MultipathSurface (rsmultipath.h)
	class MultipathSurface;

	/// A pulse sent out by a transmitter
	struct TransmitterPulse
	{
		RadarSignal* wave; //!< Base radar waveform
		RS_FLOAT time; //!< The start time of the pulse
	};

	/// Base class for transmitter and receiver
	class Radar : public Object
	{
	public:
		/// Constructor
		Radar(const Platform* platform, const std::string& name);

		/// Destructor
		~Radar() override;

		/// Set the transmitter's antenna
		void setAntenna(const Antenna* ant);

		/// Return the antenna gain in the specified direction
		RS_FLOAT getGain(const SVec3& angle, const SVec3& refangle, RS_FLOAT wavelength) const;

		/// Get the noise temperature (including antenna noise temperature)
		virtual RS_FLOAT getNoiseTemperature(const SVec3& angle) const;

		/// Make the radar monostatic
		void makeMonostatic(const Radar* recv);

		/// Get the attached radar, if we are monostatic
		const Radar* getAttached() const;

		/// Return whether the radar has an attached receiver/transmitter
		bool isMonostatic() const;

		/// Attach a timing object to the radar
		void setTiming(Timing* tim);

		/// Get a pointer to the radar's timing object
		Timing* getTiming() const;

		/// Is this object a virtual multipath dual?
		bool isMultipathDual() const;

		/// Set this object as a virtual multipath dual
		void setMultipathDual(RS_FLOAT reflect);

		/// Get the reflecting factor
		RS_FLOAT multipathDualFactor() const;

	protected:
		Timing* _timing; //!< The radar's timing source
	private:
		const Antenna* _antenna; //!< The radar's antenna
		const Radar* _attached; //!< Other radar which shares antenna (0 if not monostatic)
		bool _multipath_dual; //!< This is a virtual radar which exists for multipath simulation
		RS_FLOAT _multipath_reflect; //!< The fraction of signal power which is reflected by the multipath surface
		/// Functions which create multipath duals
		friend Receiver* createMultipathDual(Receiver* recv, const MultipathSurface* surf);

		friend Transmitter* createMultipathDual(Transmitter* trans, const MultipathSurface* surf);
	};

	//Forward declaration of Receiver
	class Receiver;

	/// A single radar transmitter, base class for pulsed and CW transmitters
	class Transmitter final : public Radar
	{
	public:
		/// Constructor
		Transmitter(const Platform* platform, const std::string& name, bool pulsed);

		/// Destructor
		~Transmitter() override;

		/// Set the transmitter's pulse waveform
		void setWave(RadarSignal* pulse);

		/// Return the number of pulses which will be sent out over the course of the simulation
		int getPulseCount() const;

		/// Fill the pulse structure with the specified pulse
		void getPulse(TransmitterPulse* pulse, int number) const;

		/// Set the Pulse Repetition Frequency of the transmitter
		void setPrf(RS_FLOAT mprf);

	protected:
		RadarSignal* _signal; //!< Waveform of transmitted pulses
		RS_FLOAT _prf{}; //!< Transmitter pulse repetition frequency (PRF)
		bool _pulsed; //!< Is this a pulsed transmitter?
		Transmitter* _dual; //!< Multipath dual of this transmitter
		/// Function to create multipath duals
		friend Transmitter* createMultipathDual(Transmitter* trans, const MultipathSurface* surf);
	};

	//Receiver objects contain receivers - the system that receives pulses
	//Receivers model the effects of their antenna
	//They also play a role in the simulation by keeping a list of responses (pulses they
	//recieve) which is built during the first part of the simulation and used to render
	//the result during the second phase.
	class Receiver final : public Radar
	{
	public:
		/// Enum type for the receiver behaviour flags
		enum RecvFlag { FLAG_NODIRECT = 1, FLAG_NOPROPLOSS = 2 };

		/// Constructor
		explicit Receiver(const Platform* platform, const std::string& name = "defRecv");

		~Receiver() override;

		/// Add a response to the list of simulation responses
		void addResponse(Response* response);

		/// Clear the list of simulation responses (deleting responses)
		void clearResponses();

		/// Export the results of the simulation to file
		void render();

		/// Get the noise temperature (including antenna noise temperature)
		RS_FLOAT getNoiseTemperature(const SVec3& angle) const override;

		/// Get the receiver noise temperature
		RS_FLOAT getNoiseTemperature() const;

		/// Set the noise temperature of the receiver
		void setNoiseTemperature(RS_FLOAT temp);

		/// Set the length of the receive window
		void setWindowProperties(RS_FLOAT length, RS_FLOAT prf, RS_FLOAT skip);

		/// Return the number of responses
		int countResponses() const;

		/// Get the number of receive windows in the simulation time
		int getWindowCount() const;

		/// Get the start time of the next window
		RS_FLOAT getWindowStart(int window) const;

		/// Get the length of the receive window
		RS_FLOAT getWindowLength() const;

		/// Get the time skipped before the start of the receive window
		RS_FLOAT getWindowSkip() const;

		/// Get the PRF
		RS_FLOAT getPrf() const;

		/// Set a flag
		void setFlag(RecvFlag flag);

		/// Check if a flag is set
		bool checkFlag(RecvFlag flag) const;

	private:
		/// Vector to hold all the system responses
		std::vector<Response*> _responses;
		boost::try_mutex _responses_mutex; //!< Mutex to serialize access to responses
		RS_FLOAT _noise_temperature; //!< Noise temperature of the receiver
		RS_FLOAT _window_length{}; //!< Length of the receive window (seconds)
		RS_FLOAT _window_prf{}; //!< Window repetition frequency
		RS_FLOAT _window_skip{}; //!< The amount of time at the beginning of an interval to skip before capturing response
		Receiver* _dual; //!< Multipath dual of this receiver
		int _flags; //!< Flags which control receiver behaviour
		/// Function to create dual
		friend Receiver* createMultipathDual(Receiver* recv, const MultipathSurface* surf);
	};

	// Functions for duplicating receivers and transmitters to create duals for multipath simulation
	Receiver* createMultipathDual(Receiver* recv, const MultipathSurface* surf);

	Transmitter* createMultipathDual(Transmitter* trans, const MultipathSurface* surf);
}

#endif
