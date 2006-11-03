//rsradarwaveform.cpp
//Classes for different types of radar waveforms
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 24 May 2006

#include <cmath>
#include <stdexcept>
#include "rsradarwaveform.h"
#include "rsparameters.h"
#include "rsdebug.h"
#include "rsnoise.h"
#include "rssignal.h"
#include <fftwcpp/fftwcpp.h>

using namespace rs;
using namespace rsRadarWaveform;

//
//RadarWaveform implementation
//

//Default constructor
RadarWaveform::RadarWaveform(std::string name, rsFloat power, rsFloat carrierfreq):
  power(power), 
  carrierfreq(carrierfreq), 
  name(name)
{

}

//Destructor
RadarWaveform::~RadarWaveform()
{

}

//Return the power of the signal
rsFloat RadarWaveform::GetPower() const
{
  return power;
}

//Get the carrier frequency
rsFloat RadarWaveform::GetCarrier() const
{
  return carrierfreq;
}

//Get the name of the pulse
std::string RadarWaveform::GetName() const
{
  return name;
}

//Get the native sample rate of the pulse
rsFloat RadarWaveform::GetRate() const
{
  return 2*carrierfreq;
}

//Default constructor (private)
RadarWaveform::RadarWaveform()
{
}

//
// PulseWaveform Implementation
//

PulseWaveform::PulseWaveform(std::string name, rsFloat length, rsFloat power, rsFloat carrierfreq):
  RadarWaveform(name, power, carrierfreq),
  length(length)
{
}

PulseWaveform::~PulseWaveform()
{
}

//Get the length of the pulse
rsFloat PulseWaveform::GetLength() const
{
  return length;
}

//Set the length of the pulse
void PulseWaveform::SetLength(rsFloat len)
{
  length = len;
}

//
// CWWaveform implementation
//

/// Constructor
CWWaveform::CWWaveform(std::string name, rsFloat power, rsFloat carrierfreq):
  RadarWaveform(name, power, carrierfreq)
{
}

/// Destructor
CWWaveform::~CWWaveform()
{
}

/// This should never be called - throw an exception
rsFloat CWWaveform::GetLength() const
{
  throw std::logic_error("[ERROR] GetLength() called on CW Waveform");
}

//
// AnyPulse implementation
//

/// Constructor
AnyPulse::AnyPulse(std::string name, rsFloat length, rsFloat power, rsFloat carrierfreq, Signal *signal):
  PulseWaveform(name, length, power, carrierfreq),
  signal(signal)
{
}

/// Destructor
AnyPulse::~AnyPulse()
{
  delete signal;
}

/// Render the waveform to the target buffer
boost::shared_array<rs::rsComplex> AnyPulse::Render(rsFloat& rate, const rs::RenderParams &params, unsigned int &size) const
{
  size = signal->Size();
  rate = signal->Rate();
  rs::rsComplex* data = signal->CopyData();  
  // Get the time shift in number of samples
  rsFloat sample_shift = params.start*signal->Rate();
  // The shift in number of samples has is some integer n plus some real part p. Shifting by an integer number of samples is trivial, but the real part is harder, so we do that here.
  rsFloat real_shift = sample_shift - std::floor(sample_shift); 
  rsSignal::TimeShift(data, size, real_shift);  
  //Render the signal into I and Q
  rsSignal::complex* iq_result = rsSignal::IQDemodulate(data, size, std::sqrt(GetPower()*params.power)/static_cast<double>(size), params.phase);
  FFTAlignedFree(data);
  // Add the doppler shift to the signal
  rsSignal::complex *ds_result = rsSignal::DopplerShift(iq_result, params.doppler, size);
    
  FFTAlignedFree(iq_result);

  return boost::shared_array<rs::rsComplex>(ds_result);
}

/// Get the amount of padding at the beginning of the pulse
rsFloat AnyPulse::GetPad() const
{
  return signal->Pad()/signal->Rate();
}

//
//MonoPulse implementation
//

MonoPulse::MonoPulse(std::string name, rsFloat length, rsFloat power, rsFloat carrierfreq):
  PulseWaveform(name, length, power, carrierfreq)
{
}

boost::shared_array<rsComplex> MonoPulse::Render(rsFloat& rate, const RenderParams &params, unsigned int &size) const
{
  // Calculate the rate
  if (rsParameters::rate() != 0)
    rate = rsParameters::rate();
  else
    rate = 2/(GetLength());
  rsDebug::printf(rsDebug::RS_VERBOSE, "[VERBOSE] Rendering with rate %f, default rate %f\n", rate, rsParameters::rate());
  // Complain if the sample rate is insufficient to express the doppler
  if (rate < std::fabs(2*params.doppler))
    rsDebug::printf(rsDebug::RS_IMPORTANT, "[IMPORTANT] Doppler frequency %f is too high to express with baseband sample rate %f. Aliasing will occur and the results will be incorrect.", params.doppler, rate);
  //Calculate the timestep
  rsFloat timestep = 1/rate;
  //Calculate the number of samples required
  rsFloat steps = std::ceil(GetLength()/timestep)-1;
  //Allocate memory for the pulse to occupy
  size = static_cast<int>(steps);
  boost::shared_array<rsComplex> result(new rsComplex[size]);
  //Get a raw pointer to work with
  rsComplex *raw = result.get();
  //Calculate the noise power
  double noise_power = rsNoise::NoiseTemperatureToPower(params.noise_temperature, rate/2.0);
  //Create the noise generator object for rendering
  WGNGenerator* noisegen = 0;
  if (noise_power != 0)
    noisegen = new WGNGenerator(std::sqrt(noise_power));

  rsFloat time = params.start;
  for (int i = 0; i < steps; i++)
    {
      rsFloat I = std::sqrt(GetPower()*params.power)*std::cos(params.phase)*cos(time*2*M_PI*params.doppler);
      rsFloat Q = std::sqrt(GetPower()*params.power)*std::sin(params.phase)*cos(time*2*M_PI*params.doppler);
      //Add noise to each of the samples
      if (noisegen) 
        {
          I += noisegen->GetSample();
          Q += noisegen->GetSample();
        }
      raw[i] = rsComplex(I, Q);
      time += timestep;
    }
  return result;
}

MonoPulse::~MonoPulse()
{  
}

/// Get the amount of padding at the beginning of the pulse
rsFloat MonoPulse::GetPad() const
{
  return 0;
}

//
//CWMonochrome Implementation
//

/// Constructor
CWMonochrome::CWMonochrome(std::string name, rsFloat power, rsFloat carrierfreq):
  CWWaveform(name, power, carrierfreq)
{
}

/// Destructor
CWMonochrome::~CWMonochrome()
{
}

/// Render the pulse into the buffer
boost::shared_array<rsComplex> CWMonochrome::Render(const std::vector<rs::CWInterpPoint> &points, unsigned int &size) const
{
  //Calculate the length of the response
  rsFloat time = points.back().time-points.front().time;
  rsFloat carrier = GetCarrier();
  //Calculate the timestep
  rsFloat rate;
  if (rsParameters::rate() != 0)
    rate = rsParameters::rate();
  else
    rate = 2*carrier;
  rsFloat timestep = 1/(rate);
  //Allocate memory for the pulse to occupy
  size = static_cast<int>(std::floor(time*(1/timestep)));
  boost::shared_array<rsComplex> result(new rsComplex[size]);
  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Render Start: %g, End: %g Size: %d\n", points.front().time, points.back().time, size);
  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Renderer of CWMonochrome allocated %g kilobytes\n", size*sizeof(rsFloat)/1024.0); 
  //Get a raw pointer to work with
  rsComplex *raw = result.get();
  unsigned int sample = 0;
  //Render each interp point
  std::vector<rs::CWInterpPoint>::const_iterator iter = points.begin();
  std::vector<rs::CWInterpPoint>::const_iterator next = iter+1;
  while (next != points.end())
    {
      rsFloat steptime = (*next).time-(*iter).time;
      rsFloat steps = std::ceil(steptime/timestep)-1;
      rsFloat start = (*iter).time;
      for (int i = 0; i < steps; i++) {
	// Check we aren't over running our array
	if (sample >= size)
	  throw std::logic_error("[BUG] Render CWMonochrome Overrunning array bounds");
	//Calculate the factor for linear interpolation
	rsFloat lif = i/steps;
	rsFloat rif = 1-lif;
	//Calculate amplitude, phase, etc
	rsFloat power = (*iter).power*rif + (*next).power*lif;
	rsFloat phase = (*iter).phase*rif + (*next).phase*lif;
	rsFloat doppler = (*iter).phase*rif + (*next).phase*lif;
	rsFloat time = start + i*timestep;
	// Set the value of the sample
	rsFloat I = std::sqrt(GetPower()*power)*std::cos(phase)*std::cos(time*2*M_PI*doppler);
	rsFloat Q = std::sqrt(GetPower()*power)*std::sin(phase)*std::cos(time*2*M_PI*doppler);
	raw[sample++] = rsComplex(I, Q);
      }
      iter = next;
      next = iter+1;
    }
  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "size %d sample %d vec %d\n", size, sample, points.size());
  return result;
}
