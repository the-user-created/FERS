//signal.cpp
//Interface for the signal class
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started 24 May 2006

#ifndef __SIGNAL_H
#define __SIGNAL_H

#include <config.h>
#include <complex>
#include <string>
#include <boost/utility.hpp>
#include <boost/shared_array.hpp>

///Forward declarations
namespace rs {
class Signal; //rssignal.h
}

namespace rsSignal {
  /// Type for storing signal
  typedef std::complex<rsFloat> complex;
  
  // Functions to process frequency domain signals
  /// Shift a signal in frequency by shift Hz
  void FrequencyShift(rsSignal::complex* data, rsFloat fs, unsigned int size, rsFloat shift);
  /// Shift a signal in time by shift seconds
  void TimeShift(rsSignal::complex *data, rsFloat fs, unsigned int size, rsFloat shift);
  /// Add noise to the signal with the given temperature
  void AddNoise(rsSignal::complex *data, rsFloat temperature, unsigned int size, double fs);
  /// Demodulate a frequency domain signal into time domain I and Q
  boost::shared_array<rsSignal::complex> IQDemodulate(rsSignal::complex *data, unsigned int size, rsFloat scale);
  ///Simulate a doppler shift of the given factor using libsamplerate
  complex* SRCDopplerShift(rsSignal::complex* data, rsFloat factor, unsigned int& size); //in rssrcdoppler.cpp
  ///Simulate a doppler shift of the given factor
  rsSignal::complex* DopplerShift(complex *data, rsFloat factor, unsigned int& size);
};

namespace rs {



  /// Class to store and process a time domain signal
class Signal: boost::noncopyable {
public:
  Signal(); //!< Default constructor
  ~Signal(); //!< Default destructor

  /// Clear deletes the data currently associated with the signal
  void Clear();
  /// Load data into the signal (time domain, real)
  void Load(rsFloat *indata, unsigned int samples, rsFloat samplerate);
  /// Load data into the signal (frequency domain, complex)
  void Load(rsSignal::complex *indata, unsigned int samples, rsFloat samplerate);
  /// Return the sample rate of the signal
  rsFloat Rate() const;
  /// Return the size, in samples of the signal
  unsigned int Size() const;
  /// Get a copy of the frequency domain data
  rsSignal::complex* CopyData() const;
  /// Get the number of samples of padding at the beginning of the pulse
  unsigned int Pad() const;
  /// Set the signal to point at new data
  void Reset(rsSignal::complex *newdata, unsigned int newsize, rsFloat newrate);
 private:
  /// The signal data (in the frequency domain)
  rsSignal::complex* data;
  /// Size of the signal in samples
  unsigned int size;  
  /// The sample rate of the signal in the time domain
  rsFloat rate;
  /// Number of samples of padding at the beginning of the pulse
  unsigned int pad;
};

}
#endif
