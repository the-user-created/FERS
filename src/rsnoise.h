//rsnoise.h Functions and classes to generate noise of various types
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//14 August 2006

#ifndef __RSNOISE_H
#define __RSNOISE_H

#include <config.h>
#include <boost/random/variate_generator.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/utility.hpp>
#include <vector>
#include "rspython.h"


// Forward Definitions
namespace rs {
class DSPFilter; //rsdsp.h
}

namespace rsNoise {
  /// Initialize the random number generator code (must be called once, after the loading of the script)
  void InitializeNoise();
  /// Clean up the noise code
  void CleanUpNoise();
  /// Return a single sample of white gaussian noise
  rsFloat WGNSample(rsFloat stddev);
  /// Return a single uniformly distributed sample in [0, 1]
  rsFloat UniformSample();
  /// Calculate noise power from the temperature
  rsFloat NoiseTemperatureToPower(rsFloat temperature, rsFloat bandwidth);
}

namespace rs {

/// NoiseGenerator - base class for different types of noise generator
class NoiseGenerator: boost::noncopyable {
public:
  /// Constructor
  NoiseGenerator();
  /// Destructor
  virtual ~NoiseGenerator();
  /// Get a single random sample
  virtual rsFloat GetSample() = 0;
};

/// Generator of Gaussian white noise
class WGNGenerator: public NoiseGenerator {
public:
  /// Constructor
  WGNGenerator(rsFloat stddev);
  /// Default Constructor
  WGNGenerator();
  /// Destructor
  ~WGNGenerator();
  /// Get a single random sample
  rsFloat GetSample();
private:
  boost::normal_distribution<rsFloat> dist; //!< PRNG distribution
  boost::variate_generator<boost::mt19937&, boost::normal_distribution<> > *gen; //!< Variate Generator (see boost::random docs)
  rsFloat temperature; //!< Noise temperature
};

/// Generator of 1/f noise using the Voss-McCartney algorithm
 class OofGenerator: public NoiseGenerator {
 public:
   /// Constructor
   OofGenerator(int needed);
   /// Destructor
   ~OofGenerator();
   /// Get a single noise sample
   rsFloat GetSample();
 private:
   int num_rows; //!< The number of 'rows' used in the algorithm
   long num_samples; //!< The number of samples we have given so far
   rsFloat *rows; //!< The number in each 'row' of the algorithm
   rsFloat running_total; //!< Running total, used to improve performance
   rsFloat scale; //!< The value to scale by to normalize the standard deviation
   WGNGenerator generator; //!< White noise generator
 };

  /// Generator of 1/f^2 noise
  class Oof2Generator: public NoiseGenerator {
  public:
    /// Constructor
    Oof2Generator(rsFloat amplitude);
    /// Destructor
    ~Oof2Generator();
    /// Get a single noise sample
    rsFloat GetSample();
  private:
    rsFloat filter_state; //!< Current state of the IIR filter
    rsFloat amplitude; //!< Mean difference in returns
    WGNGenerator generator; //!< White noise generator
  };

  /// Generator of 1/f^3 noise
  class Oof3Generator: public NoiseGenerator {
  public:
    /// Constructor
    Oof3Generator(int needed, rsFloat amplitude);
    /// Destructor
    ~Oof3Generator();
    /// Get a single noise sample
    rsFloat GetSample();
  private:
    rsFloat filter_state; //!< Current state of the IIR filter
    rsFloat amplitude; //!< Mean difference in returns
    OofGenerator generator; //!< 1/f noise generator
  };

  class ClockModelGenerator;

  /// Generator for generalized 1/f^alpha noise
  class FAlphaGenerator: public NoiseGenerator {
  public:
    /// Constructor
    FAlphaGenerator(rsFloat alpha, rsFloat amplitude, int block_size);
    /// Destructor
    ~FAlphaGenerator();
    /// Get a single noise sample
    rsFloat GetSample();
  private:
    /// Fill the sample buffer with filtered samples
    void FillSampleBuffer();
    rs::DSPFilter *filter; //!< Filter to shape the noise to the desired spectrum
    rsFloat amplitude; //!< Amplitude of created noise
    WGNGenerator generator; //!< White noise generator
    rsFloat* buffer; //!< Buffer of samples, as generating single samples is inefficient
    int buffer_size; //!< The size of the sample buffer, in samples
    int buffer_offset; //!< The offset into the sample buffer

    int total_offset; //!< Offset into the total number of samples
    int total_size; //!< The size of the total number of samples to be requested
  };

  /// Class to generate noise based on the weighted sum of 1/f^alpha noise
  class ClockModelGenerator: public NoiseGenerator {
  public:
    /// Constructor
    ClockModelGenerator(const std::vector<rsFloat> &alpha, const std::vector<rsFloat> &weights, rsFloat offset, rsFloat trend_end, int total_size, bool trend_remove);
    /// Destructor
    ~ClockModelGenerator();
    /// Get a single noise sample
    rsFloat GetSample();
  private:
    /// Refill the buffer of noise samples
    rsFloat offset; //!< 'DC' offset of the created sequence
    rsFloat trend_end; //!< End of the linear trend added to the model
    std::vector<NoiseGenerator*> generators; //!< Generators for each type of noise

    int total_offset; //!< Offset into the total number of samples
    int total_size; //!< The total number of samples to be produced
  };

  /// Generator of noise using a python module
  class PythonNoiseGenerator: public NoiseGenerator {
  public:
    ///Constructor
    PythonNoiseGenerator(const std::string& module, const std::string& function);
    ///Destructor
    ~PythonNoiseGenerator();
    ///Get a single noise sample
    rsFloat GetSample();
  private:
    rsPython::PythonNoise generator;
  };


}

#endif
