//rsnoise.cpp - Functions for generating different types of noise
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//14 August 2006

#include <cmath>
#include <limits>
#include <boost/random.hpp>
#include "rsnoise.h"
#include "rsparameters.h"


using namespace rs;

namespace {
//Use the Mersenne Twister PRNG with parameter 19937, for why this is a good choice see 
//Mersenne Twister: A 623-dimensionally equidistributed uniform pseudo-random number generator
//Matsumoto et al.
//ACM Transactions on Modeling and Computer Simulation
//January 1998
boost::mt19937* rng;

  //Used to generate single noise samples
  boost::normal_distribution<rsFloat> nd;
  boost::variate_generator<boost::mt19937&, boost::normal_distribution<rsFloat> > normal_vg(*rng, nd);
}

//
// Implementation of non-class functions
//

/// Initialize the random number generator code (must be called once, after the loading of the script)
void rsNoise::InitializeNoise()
{
  delete rng;
  rng = new boost::mt19937(rsParameters::random_seed());
}

/// Clean up the noise code
void rsNoise::CleanUpNoise()
{
  delete rng;
}

/// Return a single sample of white gaussian noise
rsFloat rsNoise::WGNSample(rsFloat stddev)
{
  if (stddev > std::numeric_limits<rsFloat>::epsilon())
    return normal_vg();
  else
    return 0;
}

/// Calculate noise amplitude from the temperature
rsFloat rsNoise::NoiseTemperatureToPower(rsFloat temperature, rsFloat bandwidth)
{
  return rsParameters::boltzmann_k()*temperature*bandwidth; //See equations.tex
}

//
// NoiseGenerator Implementation
//

//Constructor
NoiseGenerator::NoiseGenerator()
{
}

//Destructor
NoiseGenerator::~NoiseGenerator()
{
}

//
// WGNGenerator Implementation
//

//Constructor
WGNGenerator::WGNGenerator(rsFloat stddev)
{
  dist = boost::normal_distribution<rsFloat>(0, stddev);
  gen = new boost::variate_generator<boost::mt19937&, boost::normal_distribution<rsFloat> >(*rng, dist);
}

// Destructor
WGNGenerator::~WGNGenerator()
{
  delete gen;
}

// Get a sample from the rng
rsFloat WGNGenerator::GetSample()
{
  return (*gen)();
}
