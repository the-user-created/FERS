//rsmain.cpp
//Contains the main function and some support code for FERS
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 25 April 2006

#include <stdexcept>
#include "rsworld.h"
#include "xmlimport.h"
#include "rsdebug.h"
#include "rsthreadedsim.h"
#include "rsnoise.h"
#include <fftwcpp/fftwcpp.h>

/// FERS main function
int main(int argc, char *argv[])
{
  if (argc != 2) {
    rsDebug::printf(rsDebug::RS_CRITICAL, "Usage: %s <scriptfile>\n", argv[0]);
    return 2;
  }
  try {
    rs::World *world = new rs::World();
    DEBUG_PRINT(rsDebug::RS_VERBOSE, "[VERBOSE] Loading XML Script File");
    xml::LoadXMLFile(argv[1], world);
    //Initialize the RNG code
    rsNoise::InitializeNoise();
    DEBUG_PRINT(rsDebug::RS_VERBOSE, "[VERBOSE] Running Simulation Phase 1");
    rs::RunThreadedSim(1, world);
    DEBUG_PRINT(rsDebug::RS_VERBOSE, "[VERBOSE] Cleaning up");

    delete world;
  }
  catch (std::exception &ex)
    {
      rsDebug::printf(rsDebug::RS_CRITICAL, "[ERROR] Simulation encountered unexpected error:\n\t%s\nSimulator will terminate.\n", ex.what());
      return 1;
    }
  //Clean up singleton objects
  FFTCleanUp();
  rsNoise::CleanUpNoise();
  return 0;
}
