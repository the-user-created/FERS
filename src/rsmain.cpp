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
#include "rsworld.h"
#include "xmlimport.h"

/// FERS main function
int main(const int argc, char* argv[])
{
	rs_debug::printf(rs_debug::RS_CRITICAL, "/------------------------------------------------\\\n");
	rs_debug::printf(rs_debug::RS_CRITICAL, "| FERS - The Flexible Extensible Radar Simulator |\n");
	rs_debug::printf(rs_debug::RS_CRITICAL, "| Version 0.28                                   |\n");
	rs_debug::printf(rs_debug::RS_CRITICAL, "\\------------------------------------------------/\n\n");

	if (argc != 2 || !strncmp(argv[1], "--help", 6))
	{
		rs_debug::printf(rs_debug::RS_CRITICAL, "Usage: %s <scriptfile> (Run simulation specified by script file)\n",
		                argv[0]);
		rs_debug::printf(rs_debug::RS_CRITICAL, "Usage: %s --help (Show this message)\n\n", argv[0]);
		return 2;
	}
	try
	{
		// Set the number of threads
		rs::RsParameters::modifyParms()->setThreads(rs_portable::countProcessors());
		// Create the world container
		auto* world = new rs::World();
		//Initialize the RNG code
		rs_noise::initializeNoise();

		rs_debug::printf(rs_debug::RS_VERBOSE, "[VERBOSE] Loading XML Script File.\n");
		//Load the script file
		xml::loadXmlFile(argv[1], world);

		//Start the threaded simulation
		runThreadedSim(rs::RsParameters::renderThreads(), world);
		rs_debug::printf(rs_debug::RS_VERBOSE, "[VERBOSE] Cleaning up.\n");
		//Clean up the world model
		delete world;
		//Clean up singleton objects
		rs_noise::cleanUpNoise();

		rs_debug::printf(rs_debug::RS_CRITICAL, "------------------------------------------------\n");
		rs_debug::printf(rs_debug::RS_CRITICAL, "Simulation completed successfully...\n\n");

		return 0;
	}
	catch (std::exception& ex)
	{
		rs_debug::printf(rs_debug::RS_CRITICAL,
		                "[ERROR] Simulation encountered unexpected error:\n\t%s\nSimulator will terminate.\n",
		                ex.what());
		return 1;
	}
}
