//Test the noise generation code from fers
#include <config.h>
#include <iostream>
#include <boost/thread.hpp>
#include "fftwcpp.h"
#include "rsnoise.h"

using namespace rs;

unsigned int processors = 4;

//test noise generation in threaded code

void NoiseTest() {
  std::vector<rsFloat> alphas;
  std::vector<rsFloat> weights;
  //Initialize for standard five parameter clock model
  for (int i = 0; i < 5; i++) {
    alphas.push_back(i);
    weights.push_back(1);
  }
  ClockModelGenerator *gen = new ClockModelGenerator(alphas, weights, 0, 0, (int)1e6, true);
  for (int i = 0; i < 1e6; i++) {
    rsFloat j = gen->GetSample();
  }
  delete gen;
}


int main()
{
  //Initialize fftwcpp
  FFTInit(processors);
  //Initialize noise generation
  rsNoise::InitializeNoise();
  //Create the generator (for alpha = 2)
  FAlphaGenerator *gen = new FAlphaGenerator(2, 1, (int)1e4);
  //Print out 1000 samples for testing spectrum
  for (int i = 0; i < 1e4; i++) {
    rsFloat j = gen->GetSample();
    std::cout << j << std::endl;
  }
  delete gen;
  // Spawn threads to test thread safety of ClockModelGenerator
  boost::thread_group man;
  for (unsigned int i = 0; i < processors; i++) {
    man.create_thread(NoiseTest);
  }
  man.join_all();

  //Clean up
  rsNoise::CleanUpNoise();
  FFTCleanUp();
}
