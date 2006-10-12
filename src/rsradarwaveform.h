//rsradarwaveform.h
//Classes for different types of radar waveforms
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//8 June 2006

#ifndef __RS_RADARWAVEFORM_H
#define __RS_RADARWAVEFORM_H

#include <config.h>
#include <string>
#include <vector>
#include <complex>
#include <boost/shared_array.hpp>

namespace rs {

  ///Forward Declarations
  class Signal; //rssignal.h

  ///Complex type for rendering operations
  typedef std::complex<rsFloat> rsComplex;

/// A continuous wave response interpolation point
struct CWInterpPoint {
  /// Constructor
  CWInterpPoint(rsFloat power, rsFloat delay, rsFloat doppler, rsFloat phase, rsFloat noise_temperature);
  rsFloat power;
  rsFloat time;
  rsFloat doppler;
  rsFloat phase;
  rsFloat noise_temperature;
};

/// Render parameters for pulse rendering
 struct RenderParams {
   rsFloat power; //Peak power of the pulse (into 1ohm)
   rsFloat phase; //Phase (radians)
   rsFloat doppler; //Doppler shift (radians)
   rsFloat start; //Start time (seconds)
   rsFloat noise_temperature; //Noise temperature (kelvin)
 };

/// The RadarWaveform class stores information about a pulse shape (or continuous wave wave)
 class RadarWaveform
 {
 public:
   /// Default constructor
   RadarWaveform(std::string name, rsFloat power = 1, rsFloat carrierfreq = 0);
   /// Destructor
   virtual ~RadarWaveform();
   /// Get the signal power
   rsFloat GetPower() const;
   /// Get the signal carrier frequency (Hz)
   rsFloat GetCarrier() const;
   /// Get the name of the signal
   std::string GetName() const;
   /// Return the native sample rate of the waveform
   rsFloat GetRate() const;
   /// Return the length of the pulse (default implementation returns zero)
   virtual rsFloat GetLength() const = 0;
 private:   
   rsFloat power; //!< Power of the signal transmitted (in Watts)
   rsFloat carrierfreq; //!< Used for the narrowband approximation
   std::string name; //!< The name of the pulse
   /// Default constructor
   RadarWaveform();
 };

 /// Continuous wave radar waveform
 class CWWaveform: public rs::RadarWaveform
   {
   public:
     /// Constructor
     CWWaveform(std::string name, rsFloat power, rsFloat carrierfreq);
     /// Destructor
     virtual ~CWWaveform();
     /// Render the waveform to a target buffer
     virtual boost::shared_array<rsComplex> Render(const std::vector<CWInterpPoint> &points, unsigned int &size) const = 0;
     /// Length is undefined for CW pulsese
     rsFloat GetLength() const;
   private:
     CWWaveform(); //Default constructor
   };

 /// Pulse radar waveform
 class PulseWaveform: public rs::RadarWaveform
   {
   public:
     /// Constructor
     PulseWaveform(std::string name, rsFloat length, rsFloat power, rsFloat carrierfreq);
     /// Destructor
     virtual ~PulseWaveform();
     /// Render the waveform to a target buffer
     virtual boost::shared_array<rsComplex> Render(rsFloat& rate, const RenderParams &params, unsigned int &size) const = 0;
     /// Get the length of the signal (seconds)
     rsFloat GetLength() const;
     /// Get the time padding at the beginning of the pulse
     virtual rsFloat GetPad() const = 0;
   protected:
     ///Set the length of the pulse
     void SetLength(rsFloat len);
   private:
     rsFloat length; //!< The length of the wave, in seconds
     PulseWaveform(); //!< Default constructor
 };

}

namespace rsRadarWaveform {

  /// An arbitrary waveform, loaded from a file
  class AnyPulse: public rs::PulseWaveform
  {
  public:
    /// Constructor
    AnyPulse(std::string name, rsFloat length, rsFloat power, rsFloat carrierfreq, rs::Signal* signal);
    /// Destructor
    ~AnyPulse();
    /// Render the waveform to the target buffer
    boost::shared_array<rs::rsComplex> Render(rsFloat& rate, const rs::RenderParams &params, unsigned int& size) const;
    /// Get the amount of padding at the beginning of the pulse
    rsFloat GetPad() const;
  private:
    /// Default constructor
    AnyPulse();
    /// Data for pulse waveform
    rs::Signal* signal;
  };
  
  /// A single frequency pulse of the defined length
  class MonoPulse: public rs::PulseWaveform 
  {
  public:
    /// Constructor
    MonoPulse(std::string name, rsFloat length, rsFloat power = 1, rsFloat carrierfreq = 0);
    /// Destructor    
    ~MonoPulse();
    /// Render the waveform to the target buffer
    boost::shared_array<rs::rsComplex> Render(rsFloat& rate, const rs::RenderParams &params, unsigned int &size) const;
    /// Get the time of padding at the beginning of the pulse
    rsFloat GetPad() const;
  private:
    /// Default constructor
    MonoPulse();
  };

  /// A monochromatic CW waveform
  class CWMonochrome: public rs::CWWaveform
  {
  public:
    /// Constructor
    CWMonochrome(std::string name, rsFloat power, rsFloat carrierfreq);
    /// Destructor
    ~CWMonochrome();
     boost::shared_array<rs::rsComplex> Render(const std::vector<rs::CWInterpPoint> &points, unsigned int &size) const;
  private:
    CWMonochrome();
    
  };

}


#endif
