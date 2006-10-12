//rssim.cpp
//Functions which perform the actual simulations
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//30 May 2006

#include <cmath>
#include <limits>
#include <stdexcept>
#include "rssim.h"
#include "rsworld.h"
#include "rsradar.h"
#include "rsdebug.h"
#include "rsparameters.h"
#include "rsantenna.h"
#include "rstarget.h"
#include "rsresponse.h"
#include "rsnoise.h"

using namespace rs;

namespace {

/// Results of solving the bistatic radar equation and friends
struct REResults {
  rsFloat power;
  rsFloat delay;
  rsFloat doppler;
  rsFloat phase;
  rsFloat noise_temperature;
};

/// Class for range errors in RE calculations
class RangeError {
};

///Solve the radar equation for a given set of parameters
void SolveRE(const Transmitter *trans, const Receiver *recv, const Target *targ, rsFloat time, rsFloat length, RadarWaveform *wave, REResults &results)
{
  //Get the positions in space of the three objects
  Vec3 trpos = trans->GetPosition(time);
  Vec3 repos = recv->GetPosition(time);
  Vec3 tapos = targ->GetPosition(time);
  SVec3 transvec = SVec3(tapos-trpos);
  SVec3 recvvec = SVec3(tapos-repos); 
  //Calculate the distances
  rsFloat Rt = transvec.length;
  rsFloat Rr = recvvec.length;
  //Sanity check Rt and Rr and throw an exception if they are too small
  if ((Rt <= std::numeric_limits<rsFloat>::epsilon()) || (Rr <= std::numeric_limits<rsFloat>::epsilon()))
    throw RangeError();
  //Step 1, calculate the delay (in seconds) experienced by the pulse
  //See "Delay Equation" in doc/equations/equations.tex
  results.delay = (Rt+Rr)/rsParameters::c();
  //If the receiver experiences timing jitter, add it to the delay
  results.delay += rsNoise::WGNSample(recv->GetTimingJitter());
  //Get the RCS
  rsFloat RCS = targ->GetRCS(transvec, recvvec);
  //Get the wavelength
  rsFloat Wl = rsParameters::c()/wave->GetCarrier();
  //Get the system antenna gains (which include loss factors)
  rsFloat Gt = trans->GetGain(transvec, trans->GetRotation(time), Wl);
  rsFloat Gr = recv->GetGain(recvvec, recv->GetRotation(results.delay+time), Wl);

  //Step 2, calculate the received power using the narrowband bistatic radar equation
  //See "Bistatic Narrowband Radar Equation" in doc/equations/equations.tex  
  results.power = (Gt*Gr*RCS*Wl*Wl)/(pow(4*M_PI, 3)*Rt*Rt*Rr*Rr);
  //  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Pr: %2.9f Rt: %f Rr: %f Gt: %f Gr: %f RCS: %f Wl %f\n", Pr, Rt, Rr, Gt, Gr, RCS, Wl);

  //Step 3, calculate phase shift
  //See "Phase Delay Equation" in doc/equations/equations.tex
  results.phase = fmod(-results.delay*2*M_PI*wave->GetCarrier(), 2*M_PI);
  //Adjust the delay to within one phase
  results.delay = results.delay - results.phase/(2*M_PI*wave->GetCarrier());
  //rsDebug::printf(rsDebug::RS_VERBOSE, "Delay %3.15f Phase %3.15f Adj %3.15f\n", results.delay, results.phase, results.phase/(2*M_PI*wave->GetCarrier()));
  //Step 4, calculate doppler shift
  //Calculate positions at the end of the pulse
  Vec3 trpos_end = trans->GetPosition(time+length);
  Vec3 repos_end = recv->GetPosition(time+length);
  Vec3 tapos_end = targ->GetPosition(time+length);
  SVec3 transvec_end = SVec3(tapos_end-trpos_end);
  SVec3 recvvec_end = SVec3(tapos_end-repos_end); 
  rsFloat Rt_end = transvec_end.length;
  rsFloat Rr_end = recvvec_end.length;
  //Sanity check Rt_end and Rr_end and throw an exception if they are too small
  if ((Rt_end < std::numeric_limits<rsFloat>::epsilon()) || (Rr_end < std::numeric_limits<rsFloat>::epsilon()))
    throw std::runtime_error("Target is too close to transmitter or receiver for accurate simulation");
  //Doppler shift equation
  //See "Bistatic Doppler Equation" in doc/equations/equations.tex
  rsFloat vdoppler = (Rt_end-Rt+Rr_end-Rr)/length;
  results.doppler = (1+vdoppler/rsParameters::c());

  //Step 5, calculate system noise temperature
  //We only use the receive antenna noise temperature for now
  results.noise_temperature = recv->GetNoiseTemperature(recv->GetRotation(time+results.delay));
}

///Perform the first stage of simulation calcualtions for the specified pulse and target
void SimulateTarget(const PulseTransmitter *trans, Receiver *recv, const Target *targ, const World *world, const TransmitterPulse *pulse)
{
  REResults results;
  try {
    //Perform power, phase, doppler and delay calculations
    SolveRE(trans, recv, targ, pulse->time, pulse->wave->GetLength(), pulse->wave, results);
  }
  catch (RangeError re) {
    throw std::runtime_error("Receiver or Transmitter too close to Target for accurate simulation");
  }
  //Create a Response object to contain the results
  Response *response = new Response(pulse->wave, results.delay+pulse->time, results.power, results.phase, results.doppler, results.noise_temperature, trans);  
  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Adding pulse starting at %f\n", pulse->time);
  //Clip the response as required by the T-R switch on the receiver
  if (trans->ClipResponse(response, recv)) {
    //Add the response to the reciever
    recv->AddResponse(response);
  }
  else {
    delete response;
  }
}

//Perform the first stage of CW simulation calculations for the specified pulse and target
void SimulateTargetCW(const CWTransmitter *trans, Receiver *recv, const Target *targ, const World *world) {
  //Get the simulation start and end time
  rsFloat start_time = rsParameters::start_time();
  rsFloat end_time = rsParameters::end_time();
  //Calculate the number of interpolation points we need to add
  rsFloat sample_time = 1/rsParameters::cw_sample_rate();
  rsFloat point_count = (end_time-start_time)/sample_time;
  //Create the CW response
  CWResponse *response = new CWResponse(trans->GetWave(), start_time, trans);
  try {
    //Loop through and add interpolation points
    for (int i = 0; i < point_count; i++) {
      rsFloat stime = i*sample_time; //Time of the start of the sample
      //Add timing jitter
      stime += rsNoise::WGNSample(trans->GetTimingJitter());
      REResults results;
      SolveRE(trans, recv, targ, stime, sample_time, trans->GetWave(), results);
      CWInterpPoint point(results.power, start_time+results.delay, results.doppler, results.phase, results.noise_temperature);
      response->AddInterpPoint(point);
    }
  }
  catch (RangeError re) {
    throw std::runtime_error("Receiver or Transmitter too close to Target for accurate simulation");
  }
  //Clip the response as required by the T-R switch on the receiver
  if (trans->ClipResponse(response, recv)) {
    //Add the response to the reciever
    recv->AddResponse(response);
  }
  else {
    delete response;
  }  
}

/// Solve the Radar Equation and friends (doppler, phase, delay) for direct transmission
void SolveREDirect(const Transmitter *trans, const Receiver *recv, rsFloat time, rsFloat length, const RadarWaveform *wave, REResults &results)
{
  //Calculate the vectors to and from the transmitter
  Vec3 tpos = trans->GetPosition(time);
  Vec3 rpos = recv->GetPosition(time);
  SVec3 transvec = SVec3(tpos-rpos);
  SVec3 recvvec = SVec3(rpos-tpos);
  //Calculate the range
  rsFloat R = transvec.length;
  //If the two antennas are not in the same position, this can be calculated
  if (R > std::numeric_limits<rsFloat>::epsilon())
    {

      //Step 1: Calculate the delay
      //See "Delay Equation" in doc/equations/equations.tex
      results.delay = R/rsParameters::c();
      //Add the receiver's timing jitter to the delay
      results.delay += rsNoise::WGNSample(recv->GetTimingJitter());
      
      //Calculate the wavelength
      rsFloat Wl = rsParameters::c()/wave->GetCarrier();
      //Get the antenna gains
      rsFloat Gt = trans->GetGain(transvec, trans->GetRotation(time), Wl);
      rsFloat Gr = recv->GetGain(recvvec, recv->GetRotation(time+results.delay), Wl);
      //Step 2: Calculate the received power 
      //See "One Way Radar Equation" in doc/equations/equations.tex
      results.power = (Gt*Gr*Wl*Wl)/(pow(4*M_PI, 2)*pow(R, 2));

      //Step 3: Calculate the doppler shift (if one of the antennas is moving)
      Vec3 tpos_end = trans->GetPosition(time+length);
      Vec3 rpos_end = recv->GetPosition(time+length);
      Vec3 trpos_end = tpos_end - rpos_end;
      rsFloat R_end = trpos_end.Length();
      //Calculate the Doppler shift
      //See "Monostatic Doppler Equation" in doc/equations/equations.tex
      rsFloat vdoppler = (R_end-R)/length;
      results.doppler = (1+vdoppler/rsParameters::c());

      //Step 4, calculate phase shift
      //See "Phase Delay Equation" in doc/equations/equations.tex
      results.phase = fmod(results.delay*2*M_PI*wave->GetCarrier(), 2*M_PI);

      //Step 5, calculate noise temperature
      results.noise_temperature = recv->GetNoiseTemperature(recv->GetRotation(time+results.delay));
    }
  else {
    //If the receiver and transmitter are too close, throw a range error
    throw RangeError();
  }
}

/// The transmitter pulse travels directly from the transmitter to the receiver
// In a multistatic radar, the pulse is also transmitted directly from the transmitter to the receiver
void AddDirectPulse(const PulseTransmitter *trans, Receiver *recv, const World *world, const TransmitterPulse *pulse)
{  
  //If receiver and transmitter share the same antenna - there can't be a direct pulse
  if (trans->IsMonostatic() && (trans->GetAttached() == recv))
    return;
  REResults results;
  try {
    SolveREDirect(trans, recv, pulse->time, pulse->wave->GetLength(), pulse->wave, results);
  }
  catch (RangeError re) {
    //rsDebug::printf(rsDebug::RS_IMPORTANT, "Direct Response block discarded due to range error at time %g\n", pulse->time);
    return;
  }
  //Complete the response 
  Response *response = new Response(pulse->wave, results.delay+pulse->time, results.power, results.phase, results.doppler, results.noise_temperature, trans);
  //Clip the response as required by the T-R switch on the receiver
  if (trans->ClipResponse(response, recv)) {
    //Add the response to the reciever
    recv->AddResponse(response);
  }
  else {
    delete response;
  }
}

/// Model the pulse which is received directly by a reciever from a CW transmitter
void AddDirectCW(const CWTransmitter *trans, Receiver *recv, const World *world) {
  //If receiver and transmitter share the same antenna - there can't be a direct pulse
  if (trans->IsMonostatic() && (trans->GetAttached() == recv))
    return;
  //
  //Get the simulation start and end time
  rsFloat start_time = rsParameters::start_time();
  rsFloat end_time = rsParameters::end_time();
  //Calculate the number of interpolation points we need to add
  rsFloat sample_time = 1/rsParameters::cw_sample_rate();
  rsFloat point_count = (end_time-start_time)/sample_time;
  //Create the CW response
  CWResponse *response = new CWResponse(trans->GetWave(), start_time, trans);
  try {
    //Loop through and add interpolation points
    for (int i = 0; i < point_count; i++) {
      rsFloat stime = i*sample_time;
      //Add transmitter clock jitter to the time
      stime += rsNoise::WGNSample(trans->GetTimingJitter());
      REResults results;
      SolveREDirect(trans, recv, stime, sample_time, trans->GetWave(), results);
      CWInterpPoint point(results.power, start_time+results.delay+i*sample_time, results.doppler, results.phase, results.noise_temperature);
      response->AddInterpPoint(point);
    }
  }
  catch (RangeError re) {
    throw std::runtime_error("Receiver or Transmitter too close to Target for accurate simulation");
  }
  //Clip the response as required by the T-R switch on the receiver
  if (trans->ClipResponse(response, recv)) {
    //Add the response to the reciever
    recv->AddResponse(response);
  }
  else {
    delete response;
  }
}

}

//Simulate a transmitter-receiver pair with a CW transmission
void rs::SimulatePairCW(const CWTransmitter *trans, Receiver *recv, const World *world)
{
  //Add one response for each target to the receiver
  std::vector<Target*>::const_iterator targ;
  for (targ = world->targets.begin(); targ != world->targets.end(); targ++)
    SimulateTargetCW(trans, recv, *targ, world);
  // Add the direct CW reception
  AddDirectCW(trans, recv, world);
}

//Simulate a transmitter-receiver pair with a pulsed transmission
void rs::SimulatePairPulse(const PulseTransmitter *trans, Receiver *recv, const World *world)
{
  //Get the number of pulses
  int pulses = trans->GetPulseCount(rsParameters::end_time()-rsParameters::start_time());
  std::vector<Target*>::const_iterator targ;
  //Build a pulse
  TransmitterPulse* pulse = new TransmitterPulse();
  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "%d\n", pulses);
  //Loop throught the pulses
  for (int i = 0; i < pulses; i++)
    {
      trans->GetPulse(pulse, i);
      for (targ = world->targets.begin(); targ != world->targets.end(); targ++) {
	SimulateTarget(trans, recv, *targ, world, pulse);
      }
      //      Add the direct pulses
      AddDirectPulse(trans, recv, world, pulse);
    }
  delete pulse;
}

//Run a simulation on a receiver/transmitter pair
void rs::SimulatePair(const Transmitter *trans, Receiver *recv, const World *world)
{
  if (trans->GetType() == Transmitter::TRANS_PULSED) {
    const PulseTransmitter *ptrans = dynamic_cast<const PulseTransmitter*>(trans);
    if (!ptrans)
      throw std::logic_error("[BUG] Attempted pulsed simulation on a CW Transmitter");    
    SimulatePairPulse(ptrans, recv, world);
  }
  else {
    const CWTransmitter *cwtrans = dynamic_cast<const CWTransmitter*>(trans);
    if (!cwtrans)
      throw std::logic_error("[BUG] Attempted CW simulation on a pulsed Transmitter");
    SimulatePairCW(cwtrans, recv, world);
  }

}
