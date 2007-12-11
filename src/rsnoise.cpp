//rsnoise.cpp - Functions for generating different types of noise
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//14 August 2006

#include <cmath>
#include <limits>
#include <boost/random.hpp>
#include "rsnoise.h"
#include "rsdebug.h"
#include "rsparameters.h"
#include "rsdsp.h"

using namespace rs;

const static int max_fft_buffer = 100000;

namespace {
//Use the Mersenne Twister PRNG with parameter 19937, for why this is a good choice see 
//Mersenne Twister: A 623-dimensionally equidistributed uniform pseudo-random number generator
//Matsumoto et al.
//ACM Transactions on Modeling and Computer Simulation
//January 1998
boost::mt19937* rng;

  //Used to generate single noise samples
  boost::normal_distribution<rsFloat> nd(0,1);
  boost::uniform_real<rsFloat> ud(0, 1.0);
  boost::variate_generator<boost::mt19937&, boost::normal_distribution<rsFloat> >* normal_vg;
  boost::variate_generator<boost::mt19937&, boost::uniform_real<rsFloat> >* uniform_vg;
}

//
// Implementation of non-class functions
//

/// Initialize the random number generator code (must be called once, after the loading of the script)
void rsNoise::InitializeNoise()
{
  delete rng;
  delete normal_vg;
  rng = new boost::mt19937(rsParameters::random_seed());
  normal_vg = new boost::variate_generator<boost::mt19937&, boost::normal_distribution<rsFloat> >(*rng, nd);
  uniform_vg = new boost::variate_generator<boost::mt19937&, boost::uniform_real<rsFloat> >(*rng, ud);
}

/// Clean up the noise code
void rsNoise::CleanUpNoise()
{
  delete rng;
  delete normal_vg;
  delete uniform_vg;
}

/// Return a single sample of white gaussian noise
rsFloat rsNoise::WGNSample(rsFloat stddev)
{
  if (stddev > std::numeric_limits<rsFloat>::epsilon())
    return (*normal_vg)()*stddev;
  else
    return 0;
}

/// Return a single uniformly distributed sample in [0, 1]
rsFloat rsNoise::UniformSample()
{
  return (*uniform_vg)();
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

//Default constructor
WGNGenerator::WGNGenerator()
{
  dist = boost::normal_distribution<rsFloat>(0, 1);
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

//
// OofGenerator Implementation
//

// OofGenerator uses the Voss-McCartney algorithm to generate a stationary approximation of pink noise
// A in-depth analysis of its performance can be found here: http://www.firstpr.com.au/dsp/pink-noise/allan-2/spectrum2.html
// With general commentary here: http://www.firstpr.com.au/dsp/pink-noise/

/// Constructor
OofGenerator::OofGenerator(int needed)
{
  //Get the number of rows needed
  num_rows = static_cast<int>(std::ceil(std::log(needed)/std::log(2.0)));
  //Allocate memory for the rows
  rows = new rsFloat[num_rows];
  //Fill the rows with initial random values
  running_total = 0;
  for (int i = 0; i < num_rows; i++) {
    rows[i] = generator.GetSample();
    running_total += rows[i];
  }
  //Scale is equal to the square root of the number of values in the sum
  // Because the std dev is the square root of the variances, and summing normal distributions sums their variences
  scale = sqrt(num_rows+1.0);
  //We haven't handed out any samples so far
  num_samples = 0;
}

/// Destructor
OofGenerator::~OofGenerator()
{
  delete[] rows;
}

/// Get a single noise sample
rsFloat OofGenerator::GetSample()
{
  //Increase our sample count
  num_samples++;
  //Generate the sample
  rsFloat sample = (running_total + generator.GetSample())/scale;
  //Update the running total
  if (num_samples & 1)
    return sample;
  else
    {
      int temp_counter = num_samples >> 1;
      int zero_count = 0;
      while (((temp_counter & 1) == 0) && (zero_count < num_rows)) {
        temp_counter >>= 1;
        zero_count++;
      }
      //If there are too many zeros, whine about it and reset the counter
      if (num_rows <= zero_count) {
        rsDebug::printf(rsDebug::RS_IMPORTANT, "[BUG] Pink noise generator num_rows is insufficient. Simulator accuracy will suffer\n");
        num_samples = 1;
      }
      else { //Update the running total 
        running_total -= rows[zero_count];
        rows[zero_count] = generator.GetSample();
        running_total += rows[zero_count];
      }
    }
  return sample;
}

//
// Oof2Generator Implementation
//

/// Constructor
Oof2Generator::Oof2Generator(rsFloat amplitude):
  amplitude(amplitude)
{
  filter_state = generator.GetSample()*amplitude;
}

/// Destructor
Oof2Generator::~Oof2Generator()
{

}

/// Get a single noise sample
rsFloat Oof2Generator::GetSample()
{
  filter_state += generator.GetSample()*amplitude;
  return filter_state;
}


//
// Oof3Generator Implementation
//

/// Constructor
Oof3Generator::Oof3Generator(int needed, rsFloat amplitude):
  amplitude(amplitude),
  generator(needed)
{
  filter_state = generator.GetSample()*amplitude;
}

/// Destructor
Oof3Generator::~Oof3Generator()
{

}


/// Get a single noise sample
rsFloat Oof3Generator::GetSample()
{
  filter_state += generator.GetSample()*amplitude;
  return filter_state;
}

//
// FAlphaGeneator Implementation
//

/// Constructor
FAlphaGenerator::FAlphaGenerator(rsFloat alpha, rsFloat amplitude, int block_size):
  amplitude(amplitude),
  total_size(block_size)
{
  //Create the noise filter
  filter = new FAlphaFilter(alpha);
  buffer_offset = 0; //Offset of the current sample into the buffer
  total_offset = 0;
  //Limit the total maximum buffer size
  buffer_size = std::min(max_fft_buffer, total_size);
  //Allocate the sample buffer    
  buffer = new rsFloat[buffer_size];
  // Fill the sample buffer
  FillSampleBuffer();
}

/// Destructor
FAlphaGenerator::~FAlphaGenerator()
{
  delete filter;
  delete[] buffer;
}

/// Get a single noise sample
rsFloat FAlphaGenerator::GetSample()
{
  // Refill the buffer if it is exhausted
  if ((buffer_offset == buffer_size)) {
    if (total_offset < total_size) {//Total size is not exhausted
      FillSampleBuffer();
      buffer_offset = 0;
    }
    else { // Used up our maximum size - no need to generate a new block
      buffer_offset = 0;
    }
  }
  //If we are re-using samples, throw an exception
  if (total_offset >= total_size)
    throw std::logic_error("[BUG] More samples requested from FAlphaGenerator than total_size");
  //Get the sample
  rsFloat sample = buffer[buffer_offset];
  buffer_offset++;
  total_offset++;
  return sample;
}

/// Fill the sample buffer with filtered samples
void FAlphaGenerator::FillSampleBuffer()
{  
  // Fill the buffer with white noise
  for (int i = 0; i < buffer_size; i++)
    buffer[i] = generator.GetSample();

  // Filter the buffer
  filter->Filter(buffer, buffer_size);
  //Move the noise to start at zero
  rsFloat offset = buffer[0];
  for (int i = 0; i < buffer_size; i++)
    buffer[i] -= offset;  
}

//
// ClockModelGenerator Implementation
//

/// Constructor
ClockModelGenerator::ClockModelGenerator(const std::vector<rsFloat> &alpha, const std::vector<rsFloat> &weights, rsFloat offset, rsFloat trend_end, int total_size, bool trend_remove = true):
  offset(offset),
  trend_end(trend_end),
  total_size(total_size)
{
  // Check that there are equal numbers of alphas and weights
  if (alpha.size() != weights.size())
    throw std::logic_error("[BUG] ClockModelGenerator requires equal number of weights and alphas");

  // Create generators for each alpha
  std::vector<rsFloat>::const_iterator aiter = alpha.begin();
  std::vector<rsFloat>::const_iterator witer = weights.begin();
  for (; aiter != alpha.end(); aiter++, witer++) {
    NoiseGenerator *gen = new FAlphaGenerator(*aiter, *witer, total_size);
    generators.push_back(gen);
  }

  total_offset = 0;
}

/// Destructor
ClockModelGenerator::~ClockModelGenerator()
{
  // Delete each of the generators
  std::vector<NoiseGenerator *>::iterator giter = generators.begin();
  for (; giter < generators.end(); giter++)
    delete *giter;
}

/// Get a single noise sample
rsFloat ClockModelGenerator::GetSample()
{
  //Sanity check the generated samples counter
  if (total_offset >= total_size)
    throw std::logic_error("[BUG] More than total_size samples requested from GetSample()");

  rsFloat sample = 0;
  // Add noise from each generator to the sample
  std::vector<NoiseGenerator *>::const_iterator giter = generators.begin();
  for (; giter < generators.end(); giter++)
    sample += (*giter)->GetSample();
  // Add linear interpolation factors
  sample += offset + (total_offset/(rsFloat)(total_size-1.0))*trend_end;
  // Increment the generated samples counter
  total_offset++;
  return sample;
}


//
//PythonNoiseGenerator Implementation
//

///Constructor
PythonNoiseGenerator::PythonNoiseGenerator(const std::string& module, const std::string& function):
  generator(module, function)
{

}

///Destructor
PythonNoiseGenerator::~PythonNoiseGenerator()
{
}

///Get a single noise sample
rsFloat PythonNoiseGenerator::GetSample()
{
  return generator.GetSample();
}

