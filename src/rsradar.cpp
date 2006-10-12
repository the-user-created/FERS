//rsradar.cpp
//Implementation of classes defined in rsradar.h
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started 21 April 2006

#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <limits>
#include "rsradar.h"
#include "rsdebug.h"
#include "rspulserender.h"
#include "rsresponse.h"
#include "rsantenna.h"
#include "rsparameters.h"
#include "rspath.h"
#include "rsnoise.h"

using namespace rs; //Import the rs namespace for clarity

//
// Radar Implementation
//

/// Default Constructor
Radar::Radar(const Platform *platform, std::string name):
  Object(platform, name),
  antenna(0),
  attached(0),
  timing_jitter(0)
{
}

/// Default Destructor
Radar::~Radar()
{
}

/// Attach a receiver to the transmitter for a monostatic configuration
void Radar::MakeMonostatic(Radar* recv)
{
  if (attached)
    throw std::runtime_error("[BUG] Attempted to attach second receiver to transmitter");
  attached = recv;
}

/// Get the attached receiver
//Attached is likely to be 0 (NULL) - which means the transmitter does not share it's antenna
const Radar* Radar::GetAttached() const
{
  return attached;
}

/// Return whether the radar is monostatic
bool Radar::IsMonostatic() const
{
  return attached;
}

/// Set the transmitter's antenna
void Radar::SetAntenna(Antenna* ant)
{
  if (!ant)
    throw std::logic_error("[BUG] Transmitter's antenna set to null");
  antenna = ant;  
}

/// Return the antenna gain in the specified direction
rsFloat Radar::GetGain(const SVec3 &angle, const SVec3 &refangle, rsFloat wavelength) const
{
  return antenna->GetGain(angle, refangle, wavelength);
}

/// Get the noise temperature (including antenna noise temperature)
rsFloat Radar::GetNoiseTemperature(const SVec3 &angle) const
{
  return antenna->GetNoiseTemperature(angle);
}

/// Set the timing jitter (value is standard deviation of jitter, in seconds)
void Radar::SetTimingJitter(rsFloat jitter)
{
  if (jitter < -std::numeric_limits<rsFloat>::epsilon())
    throw std::logic_error("[ERROR] Timing jitter must not be less than zero.");
  timing_jitter = jitter;
}

/// Get the current timing jitter value (standard deviation of jitter)
rsFloat Radar::GetTimingJitter() const
{
  return timing_jitter;
}

//
// Transmitter Implementation
//

//Default constructor for Transmitter
Transmitter::Transmitter(const Platform *platform, std::string name):
  Radar(platform, name),
  pulseWave(0)
{
}

//Default destructor for Transmitter
Transmitter::~Transmitter()
{
}

/// Set the transmitter's pulse waveform
void Transmitter::SetWave(RadarWaveform *wave)
{
  pulseWave = wave;
}

//
//PulseTransmitter Implementation
//

// Constructor
PulseTransmitter::PulseTransmitter(const Platform *platform, std::string name):
  Transmitter(platform, name),
  pulseLength(0),
  prf(0)
{
}

//Destructor
PulseTransmitter::~PulseTransmitter()
{
}

// Return the number of pulses this transmitter produces over the simulation lifetime
int PulseTransmitter::GetPulseCount(const rsFloat time) const 
{
  //Very simple for now, but will get more complex as more features are added
  rsFloat pulses = time*prf;
  return static_cast<int>(std::ceil(pulses));
}

// Fill the structure with the number'th pulse in the transmitter's pulse list
void PulseTransmitter::GetPulse(TransmitterPulse *pulse, int number) const
{
  //Pulse waveform is same as transmitter waveform
  pulse->wave = pulseWave;
  //Calculate start time of pulse
  pulse->time = static_cast<rsFloat>(number)/prf;
  //If there is timing jitter, add it
  pulse->time += rsNoise::WGNSample(GetTimingJitter());
}

// Set the PRF of the transmitter
void PulseTransmitter::SetPRF(rsFloat mprf)
{
  prf = mprf;
}

/// Get the type of the transmitter (pulsed or CW)
Transmitter::TransmitterType PulseTransmitter::GetType() const
{
  return Transmitter::TRANS_PULSED;
}

/// Clip a response to simulate the T-R switch
bool PulseTransmitter::ClipResponse(ResponseBase *resp, const Receiver *recv) const
{

  if (IsMonostatic() && (recv == GetAttached())) {
    // Monostatic case
    rsFloat prTime = 1/prf;
    rsFloat pStart = std::fmod(resp->GetStart(), prTime);
    rsFloat pEnd = pStart + resp->GetLength();
    //If the entire pulse is deadtime, discard it
    if ((pStart < pulseWave->GetLength()) && (pEnd < pulseWave->GetLength())) {
      DEBUG_PRINT(rsDebug::RS_VERY_VERBOSE, "T-R Switch Discarded entire pulse");
      return false;
    }
    //Deadtime at the start
    if (pStart < pulseWave->GetLength())
      resp->AddDeadTime(std::make_pair(0, pulseWave->GetLength()));
    //Deadtime in the middle and at the end
    rsFloat count = 0;
    while (pEnd > prTime) {
      resp->AddDeadTime(std::make_pair(count*prTime, count*prTime+pulseWave->GetLength()));
      count++;
      pEnd -= prTime;
    }
    return true;
  }
  else {
    // Multistatic case
    // TODO: Support T-R switch for receivers with unconnected transmitters
    return true;
  }
}

//
// CWTransmitter Implementation
//

// Constructor
CWTransmitter::CWTransmitter(const Platform *platform, std::string name):
  Transmitter(platform, name)
{
}

//Destructor
CWTransmitter::~CWTransmitter()
{
}

// Return the CW waveform of the pulse
rs::RadarWaveform* CWTransmitter::GetWave() const
{
  return pulseWave;
}

// Get the type of the transmitter
Transmitter::TransmitterType CWTransmitter::GetType() const
{
  return Transmitter::TRANS_CONTINUOUS;
}

/// Clip a response to simulate the T-R switch
bool CWTransmitter::ClipResponse(ResponseBase *resp, const Receiver *recv) const
{
  //TODO: Implement this
  return true;
}

//
// Receiver Implementation
//


//Default constructor for Receiver
Receiver::Receiver(const Platform *platform, std::string name):
  Radar(platform, name),
  noise_temperature(0)
{
}

//Default destructor for Receiver
Receiver::~Receiver()
{
  ClearResponses();
}

//Add a response to the list of responses for this receiver
void Receiver::AddResponse(ResponseBase *response)
{
  boost::try_mutex::scoped_lock lock(responses_mutex);
  responses.push_back(response);
}

//Clear the list of system responses
void Receiver::ClearResponses()
{
  std::vector<ResponseBase *>::iterator i;
  for (i = responses.begin(); i != responses.end(); i++)
    delete *i;
  responses.clear();
}

/// Comparison function for response*
inline bool CompareTimes(const ResponseBase *a, const ResponseBase *b)
{
  return (a->GetStart())<(b->GetStart());
}

/// Render the antenna's responses
void Receiver::Render()
{
  try {
    // This mutex should never be locked, enforce that condition
    boost::try_mutex::scoped_try_lock lock(responses_mutex);    
    //Sort the returns into time order
    std::sort(responses.begin(), responses.end(), CompareTimes);
    //Export the pulse descriptions to XML
    if (rsParameters::export_xml())
      ExportReceiverXML(responses, GetName() + "_results");
    //Export a binary containing the pulses
    if (rsParameters::export_binary() || rsParameters::export_csvbinary())
      ExportReceiverBinary(responses, GetName(), GetName()+"_results");
    //Export to CSV format
    if (rsParameters::export_csv())
      ExportReceiverCSV(responses, GetName()+"_results");
    //Unlock the mutex
    lock.unlock();
  }
  catch (boost::lock_error e)
    {
      throw std::runtime_error("[BUG] Responses lock is locked during Render()");
    }
}

/// Get the noise temperature (including antenna noise temperature)
rsFloat Receiver::GetNoiseTemperature(const SVec3 &angle) const
{
  return noise_temperature+Radar::GetNoiseTemperature(angle);
}

/// Set the noise temperature of the receiver
void Receiver::SetNoiseTemperature(rsFloat temp)
{
  if (temp < -std::numeric_limits<rsFloat>::epsilon())
    throw std::runtime_error("Noise temperature set to negative value.");
  noise_temperature = temp;
}

/// Return the number of responses
int Receiver::CountResponses() const
{
  return responses.size();
}
