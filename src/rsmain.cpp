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
#include "fftwcpp.h"

unsigned int processors = 4; //TODO: Get real number of processors

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
    //Load the script file
    xml::LoadXMLFile(argv[1], world);
    //Init the FFT code
    FFTInit(processors);
    //Initialize the RNG code
    rsNoise::InitializeNoise();
    //Start the threaded simulation
    rs::RunThreadedSim(processors, world);
    DEBUG_PRINT(rsDebug::RS_VERBOSE, "[VERBOSE] Cleaning up");
    //Clean up the world model
    delete world;
    //Clean up singleton objects
    rsNoise::CleanUpNoise();
    FFTCleanUp();
    return 0;
  }
  catch (std::exception &ex)
    {
      rsDebug::printf(rsDebug::RS_CRITICAL, "[ERROR] Simulation encountered unexpected error:\n\t%s\nSimulator will terminate.\n", ex.what());
      return 1;
    }
  return 0;

}
