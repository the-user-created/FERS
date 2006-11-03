//rsradar.h
/// Defines classes for recievers, transmitters and antennas
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 21 April 2006
#ifndef __RSRADAR_H
#define __RSRADAR_H

#include <config.h>
#include <vector>
#include <string>
#include "rsobject.h"
#include "rsradarwaveform.h"
#include <boost/utility.hpp>
#include <boost/thread/mutex.hpp>

namespace rs {

  //Forward declaration of ResponseBase (see rsresponse.h)
  class ResponseBase;

  //Forward declaration of Antenna (see rsantenna.h)
  class Antenna;
  
  //Forward declaration of SVec3 and Vec3 (see rspath.h)
  class SVec3;
  class Vec3;

  //Forward declaration of Timing (see rstiming.h)
  class Timing;

  /// A pulse sent out by a transmitter
  struct TransmitterPulse
  {
    rs::RadarWaveform *wave; //!< Base radar waveform
    rsFloat time; //!< The start time of the pulse
  };

  /// Base class for transmitter and receiver
  class Radar: public Object
    {
    public:
      /// Constructor
      Radar(const Platform *platform, std::string name);
      /// Destructor
      virtual ~Radar();
      /// Set the transmitter's antenna
      void SetAntenna(Antenna *ant);
      /// Return the antenna gain in the specified direction
      rsFloat GetGain(const SVec3& angle, const SVec3& refangle, rsFloat wavelength) const;
      /// Get the noise temperature (including antenna noise temperature)
      virtual rsFloat GetNoiseTemperature(const SVec3& angle) const;
      /// Make the radar monostatic
      void MakeMonostatic(Radar *recv);
      /// Get the attached radar, if we are monostatic
      const Radar* GetAttached() const;
      /// Return whether the radar has an attached receiver/transmitter
      bool IsMonostatic() const;
      /// Attach a timing object to the radar
      void SetTiming(Timing* tim);
    protected:
      const Timing *timing; //!< The radar's timing source
    private:      
      const Antenna *antenna; //!< The radar's antenna
      const Radar *attached; //!< Other radar which shares antenna (0 if not monostatic)
    };

  //Forward declaration of Receiver
  class Receiver;
  
  /// A single radar transmitter, base class for pulsed and CW transmitters
  class Transmitter: public Radar
    {
    public:
      /// Supported types of transmitter model
      enum TransmitterType { TRANS_PULSED, /// Pulsed transmitter which shares an antenna with a receiver
			     TRANS_CONTINUOUS /// Continuous wave transmitter
      };  
      /// Constructor
      Transmitter(const Platform *platform, std::string name = "defTrans");
      /// Destructor
      virtual ~Transmitter();
      /// Get the type of the transmitter (pulsed or CW)
      virtual TransmitterType GetType() const = 0;
      /// Set the transmitter's pulse waveform
      void SetWave(RadarWaveform *pulse);
    protected:
      rs::RadarWaveform* pulseWave; //!< Waveform of transmitted pulses
    };

  /// A pulsed radar transmitter
  class PulseTransmitter: public Transmitter
    {
    public:
      /// Constructor
      PulseTransmitter(const Platform *platform, std::string name);
      /// Destructor
      ~PulseTransmitter();
      /// Return the number of pulses which will be sent out over the course of the simulation
      int GetPulseCount(const rsFloat time) const;
      /// Fill the pulse structure with the specified pulse
      void GetPulse(TransmitterPulse *pulse, int number) const;
      /// Set the Pulse Repetition Frequency of the transmitter
      void SetPRF(rsFloat mprf);
      /// Get the type of the transmitter (pulsed or CW)
      Transmitter::TransmitterType GetType() const;
    private:
      rsFloat pulseLength; //!< Length of the pulses sent out by the transmitter
      rsFloat prf; //!< Transmitter pulse repetition frequency (PRF)
  };

  /// A CW radar transmitter
  class CWTransmitter: public Transmitter
  {
  public:
    /// Constructor
    CWTransmitter(const Platform *platform, std::string name);
    /// Destructor
    ~CWTransmitter();
    /// Return the CW waveform of the pulse
    rs::RadarWaveform* GetWave() const;
    /// Get the type of the transmitter (pulsed or CW)
    Transmitter::TransmitterType GetType() const;
  };


  //Receiver objects contain receivers - the system that receives pulses
  //Receivers model the effects of their antenna
  //They also play a role in the simulation by keeping a list of responses (pulses they
  //recieve) which is built during the first part of the simulation and used to render
  //the result during the second phase.
  class Receiver: public Radar
    {
    public:
      Receiver(const Platform *platform, std::string name = "defRecv");
      ~Receiver();
      /// Add a response to the list of simulation responses
      void AddResponse(ResponseBase *response);
      /// Clear the list of simulation responses (deleting responses)
      void ClearResponses();
      /// Export the results of the simulation to file
      void Render();
      /// Get the noise temperature (including antenna noise temperature)
      rsFloat GetNoiseTemperature(const SVec3 &angle) const;
      /// Get the receiver noise temperature
      rsFloat GetNoiseTemperature() const;
      /// Set the noise temperature of the receiver
      void SetNoiseTemperature(rsFloat temp);
      /// Set the length of the receive window
      void SetWindowProperties(rsFloat length, rsFloat prf, rsFloat skip);
      /// Return the number of responses
      int CountResponses() const;
      /// Get the number of receive windows in the simulation time
      int GetWindowCount() const;
      /// Get the start time of the next window
      rsFloat GetWindowStart(int window) const;
      /// Get the length of the receive window
      rsFloat GetWindowLength() const;
    private:
      /// Vector to hold all the system responses
      std::vector<ResponseBase *> responses;
      boost::try_mutex responses_mutex; //!< Mutex to serialize access to responses
      rsFloat noise_temperature; //!< Noise temperature of the reciever
      rsFloat window_length; //!< Length of the receive window (seconds)
      rsFloat window_prf; //!< Window repetition frequency
      rsFloat window_skip; //!< The amount of time at the beginning of an interval to skip before capturing response
    };

}

#endif
