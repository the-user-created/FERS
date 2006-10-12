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

namespace rsNoise {
  /// Initialize the random number generator code (must be called once, after the loading of the script)
  void InitializeNoise();
  /// Clean up the noise code
  void CleanUpNoise();
  /// Return a single sample of white gaussian noise
  rsFloat WGNSample(rsFloat stddev);
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
  /// Destructor
  ~WGNGenerator();
  /// Get a single random sample
  rsFloat GetSample();
private:
  boost::normal_distribution<rsFloat> dist; //!< PRNG distribution
  boost::variate_generator<boost::mt19937&, boost::normal_distribution<> > *gen; //!< Variate Generator (see boost::random docs)
  rsFloat temperature; //!< Noise temperature
};

}

#endif
