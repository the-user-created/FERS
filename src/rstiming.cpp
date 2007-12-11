//rstiming.cpp - Implementation of timing sources

#include "rstiming.h"
#include "rsnoise.h"
#include "rsdebug.h"
#include <algorithm>

using namespace rs;

//
// Timing Implementation
//

/// Default constructor
Timing::Timing(const std::string &name):
  name(name)
{
}

/// Destructor
Timing::~Timing()
{
} 

/// Get the name of the timing source
std::string Timing::GetName() const
{
  return name;
}

//
// PrototypeTiming Implementation
//

/// Constructor
PrototypeTiming::PrototypeTiming(const std::string &name):
  name(name)
{
}
/// Add an alpha and a weight to the timing prototype
void PrototypeTiming::AddAlpha(rsFloat alpha, rsFloat weight)
{
  alphas.push_back(alpha);
  weights.push_back(weight);
}

/// Get the alphas and weights from the prototype
void PrototypeTiming::GetAlphas(std::vector<rsFloat> &get_alphas, std::vector<rsFloat> &get_weights) const
{
  //Copy the alpha and weight vectors
  get_alphas = alphas;
  get_weights = weights;
}

/// Get the name of the prototype
std::string PrototypeTiming::GetName() const
{
  return name;
}


//
// ClockModelTiming Implementation
//

/// Constructor
ClockModelTiming::ClockModelTiming(const std::string &name, int num_pulses, int pulse_length, int pulse_gap):
  Timing(name),
  enabled(false),
  num_pulses(num_pulses),
  pulse_length(pulse_length),
  pulse_gap(pulse_gap)
{
  // These are NULL for now, as initialization is not complete
  trend_samples = 0;
  model = 0;
  current_pulse = 0;
}

/// Destructor
ClockModelTiming::~ClockModelTiming() {
  delete[] trend_samples;
}

/// Get the real time of a particular pulse
rsFloat ClockModelTiming::GetPulseTimeError(int pulse) const
{
  if (enabled) {
    if (!trend_samples)
      throw std::runtime_error("[BUG] ClockModelTiming used before InitializeModel() called");
    if (pulse >= num_pulses)
      throw std::runtime_error("[BUG] Time for pulse > num_pulses requested from GetTimePulseError");
    // Return the time sample at the beginning of the time interval
    return trend_samples[pulse];
  }
  else 
    return 0.0; // Model is disabled
}

/// Get the next sample of time error for a particular pulse
rsFloat ClockModelTiming::NextNoiseSample()
{
  if (enabled) {
    if (current_pulse > num_pulses)
      throw std::runtime_error("[BUG] NextNoiseSample() called after all pulses are exhausted");
    if (!model)
      throw std::runtime_error("[BUG] ClockModelTiming used before InitializeModel() called");
    return model->GetSample();
  }
  else
    return 0; // Model is disabled
}
    
/// Advance to the next pulse
void ClockModelTiming::PulseDone() 
{ 
  if (enabled) {
    //Check the pulse number is within range
    if (current_pulse > num_pulses)
      throw std::logic_error("[BUG] PulseDone() called too many times");
    if (current_pulse == num_pulses) {
      delete model;
      current_pulse++;
    }
    else { //Create a new pulse
      // Linearly interpolate the trend line end
      rsFloat pri_length = pulse_length+pulse_gap;
      rsFloat trend_end = trend_samples[current_pulse+1]*(pulse_length/pri_length)+trend_samples[current_pulse]*((pri_length-pulse_length)/pri_length);
      //Remove the previous generator
      delete model;
      //Create a new generator
      model = new ClockModelGenerator(alphas, weights, trend_samples[current_pulse], trend_end, pulse_length, true);
      //Update the used pulse count
      current_pulse++;
    }
  }
}

/// Initialize the clock model generator
void ClockModelTiming::InitializeModel(const PrototypeTiming *timing)
{
  if (!alphas.empty())
    throw std::logic_error("[BUG] ClockModelTiming::InitializeModel called more than once");
  //Copy the alpha and weight vectors
  timing->GetAlphas(alphas, weights);
  if (!alphas.empty()) {
    //Fill the trend samples
    ClockModelGenerator trend_gen(alphas, weights, 0, 0, num_pulses+1, false);
    trend_samples = new rsFloat[num_pulses+1];
    for (int i = 0; i < num_pulses+1; i++)
      trend_samples[i] = trend_gen.GetSample();
    //Init the model for the first pulse
    PulseDone();
    //Enable the clock model
    enabled = true;
  }
}

/// Return the enabled state of the clock model
bool ClockModelTiming::Enabled()
{
  return enabled;
}

//
// PulseTiming Implementation
//

/// Constructor
PulseTiming::PulseTiming(const std::string &name, int num_pulses):
  Timing(name),
  num_pulses(num_pulses)
{
  trend_samples = 0;
}

/// Destructor
PulseTiming::~PulseTiming()
{
  delete[] trend_samples;
}

/// Get the real time of a particular pulse
rsFloat PulseTiming::GetPulseTimeError(int pulse) const
{
 if (!trend_samples)
    throw std::runtime_error("[BUG] ClockModelTiming used before InitializeModel() called");
  if (pulse >= num_pulses)
   throw std::runtime_error("[BUG] Time for pulse > num_pulses requested from GetTimePulseError");
  // Return the time sample at the beginning of the time interval
  return trend_samples[pulse];
}

/// Get the next sample of time error for a particular pulse
rsFloat PulseTiming::NextNoiseSample()
{
  throw std::logic_error("[BUG] NextNoiseSample should never be called on PulseTiming");
}

/// Advance to the next pulse
void PulseTiming::PulseDone() 
{
  throw std::logic_error("[BUG] PulseDone should never be called on PulseTiming");
}

/// Initialize the clock model generator
void PulseTiming::InitializeModel(const PrototypeTiming *timing)
{
  // Get the parameters from the timing prototype
  std::vector<rsFloat> alphas;
  std::vector<rsFloat> weights;
  timing->GetAlphas(alphas, weights);
  //Fill the trend samples
  ClockModelGenerator trend_gen(alphas, weights, 0, 0, num_pulses+1, false);
  trend_samples = new rsFloat[num_pulses+1];
  for (int i = 0; i < num_pulses+1; i++) {
    trend_samples[i] = trend_gen.GetSample();
  }
}
