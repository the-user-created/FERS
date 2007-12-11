// Test clock modeling classes
#include <config.h>
#include <iostream>
#include "fftwcpp.h"
#include "rstiming.h"

using namespace rs;

unsigned int processors = 4;

//Create the prototype timing class
PrototypeTiming* MakeProtoTiming()
{
  //Two parameter clock model
  PrototypeTiming *proto = new PrototypeTiming("test");
  proto->AddAlpha(0, 0.05); //White PM
  proto->AddAlpha(2, 0.95); //White FM
  //proto->AddAlpha(3, 0); //Flicker FM
  return proto;
}

// Test the pulsetiming class
int TestPulseTiming(const PrototypeTiming *proto)
{
  int pulses = 1e6;
  PulseTiming *timing = new PulseTiming("test_pulse", pulses);
  timing->InitializeModel(proto);
  std::cout << "---------------------" << std::endl;
  for (int i = 0; i < pulses; i++) {
    rsFloat j = timing->GetPulseTimeError(i);
    //   std::cout << j << std::endl;
  }
  delete timing;
  return 0;
}

// Test the pulsetiming class
int TestClockModelTiming(const PrototypeTiming *proto)
{
  int pulses = 3;
  int pulse_length = 1e6;
  int pulse_gap = 9e6;
  ClockModelTiming *timing = new ClockModelTiming("test_pulse", pulses, pulse_length, pulse_gap);
  timing->InitializeModel(proto);
  
  for (int i = 0; i < pulses; i++) {
    rsFloat j = timing->GetPulseTimeError(i);
    //    std::cout << j << std::endl;
  }
  for (int k = 0; k < pulses; k++) {
    for (int i = 0; i < pulse_length; i++) {
      rsFloat j = timing->NextNoiseSample();
      //      std::cout << j << std::endl;
    }
    timing->PulseDone();
  }
  delete timing;
  return 0;
}


int main()
{
  FFTInit(processors);
  rsNoise::InitializeNoise();
  PrototypeTiming *timing = MakeProtoTiming();

  //TestPulseTiming(timing);
    TestClockModelTiming(timing);

  rsNoise::CleanUpNoise();
  FFTCleanUp();
  delete timing;
}
