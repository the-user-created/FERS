//rsmain.cpp
//Contains the main function and some support code for FERS
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 25 April 2006

#include <cstring>
#include <stdexcept>

#include "rsdebug.h"
#include "rsnoise.h"
#include "rsparameters.h"
#include "rsportable.h"
#include "rsthreadedsim.h"
#include "rsworld.h"
#include "xmlimport.h"

/// FERS main function
int main(const int argc, char* argv[])
{
	rsDebug::printf(rsDebug::RS_CRITICAL, "/------------------------------------------------\\\n");
	rsDebug::printf(rsDebug::RS_CRITICAL, "| FERS - The Flexible Extensible Radar Simulator |\n");
	rsDebug::printf(rsDebug::RS_CRITICAL, "| Version 0.28                                   |\n");
	rsDebug::printf(rsDebug::RS_CRITICAL, "\\------------------------------------------------/\n\n");

	if (argc != 2 || !strncmp(argv[1], "--help", 6))
	{
		rsDebug::printf(rsDebug::RS_CRITICAL, "Usage: %s <scriptfile> (Run simulation specified by script file)\n",
		                argv[0]);
		rsDebug::printf(rsDebug::RS_CRITICAL, "Usage: %s --help (Show this message)\n\n", argv[0]);
		return 2;
	}
	try
	{
		// Set the number of threads
		rs::rsParameters::modify_parms()->SetThreads(rsPortable::CountProcessors());
		// Create the world container
		rs::World* world = new rs::World();
		//Initialize the RNG code
		rsNoise::InitializeNoise();

		rsDebug::printf(rsDebug::RS_VERBOSE, "[VERBOSE] Loading XML Script File.\n");
		//Load the script file
		xml::LoadXMLFile(argv[1], world);

		//Start the threaded simulation
		rs::RunThreadedSim(rs::rsParameters::render_threads(), world);
		rsDebug::printf(rsDebug::RS_VERBOSE, "[VERBOSE] Cleaning up.\n");
		//Clean up the world model
		delete world;
		//Clean up singleton objects
		rsNoise::CleanUpNoise();

		rsDebug::printf(rsDebug::RS_CRITICAL, "------------------------------------------------\n");
		rsDebug::printf(rsDebug::RS_CRITICAL, "Simulation completed successfully...\n\n");

		return 0;
	}
	catch (std::exception& ex)
	{
		rsDebug::printf(rsDebug::RS_CRITICAL,
		                "[ERROR] Simulation encountered unexpected error:\n\t%s\nSimulator will terminate.\n",
		                ex.what());
		return 1;
	}

	return 0;
}
