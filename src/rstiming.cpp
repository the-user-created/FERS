//rstiming.cpp - Implementation of timing sources

#include "rstiming.h"
#include "rsnoise.h"
#include "rsdebug.h"

using namespace rs;

//
// Timing Implementation
//

/// Default constructor
Timing::Timing(const std::string &name, rsFloat rate, rsFloat jitter):
  name(name),
  rate(rate),
  jitter(jitter)
{
}

/// Destructor
Timing::~Timing()
{
}

/// Get the nearest pulse to the specified time
rsFloat Timing::GetTime(rsFloat approx_time, rsFloat propagation_error) const
{
  //Model the source as a random jitter for now
  rsFloat real_time = approx_time+rsNoise::WGNSample(propagation_error)+rsNoise::WGNSample(jitter);
  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Timing->GetTime %f %f Jitter: %f\n", approx_time, real_time, jitter);
  return real_time;
}

/// Get the name of the timing source
std::string Timing::GetName() const
{
  return name;
}
