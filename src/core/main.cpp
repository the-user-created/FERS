// main.cpp
// Contains the main function and some support code for FERS
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 25 April 2006

#include <cstring>
#include <stdexcept>

#include "parameters.h"
#include "portable_utils.h"
#include "serialization/xmlimport.h"
#include "simulation/noise_generators.h"
#include "simulation/noise_utils.h"
#include "simulation/sim_threading.h"

int main(const int argc, char* argv[])
{
	logging::printf(logging::RS_CRITICAL, "/------------------------------------------------\\\n");
	logging::printf(logging::RS_CRITICAL, "| FERS - The Flexible Extensible Radar Simulator |\n");
	logging::printf(logging::RS_CRITICAL, "| Version 0.28                                   |\n");
	logging::printf(logging::RS_CRITICAL, "\\------------------------------------------------/\n\n");

	if (argc != 2 || !strncmp(argv[1], "--help", 6))
	{
		logging::printf(logging::RS_CRITICAL, "Usage: %s <scriptfile> (Run simulation specified by script file)\n",
		                argv[0]);
		logging::printf(logging::RS_CRITICAL, "Usage: %s --help (Show this message)\n\n", argv[0]);
		return 2;
	}

	try
	{
		parameters::setThreads(portable_utils::countProcessors());
		auto* world = new rs::World();
		noise_utils::initializeNoise();

		logging::printf(logging::RS_VERBOSE, "[VERBOSE] Loading XML Script File.\n");
		xml::loadXmlFile(argv[1], world);

		rs::threaded_sim::runThreadedSim(parameters::renderThreads(), world);
		logging::printf(logging::RS_VERBOSE, "[VERBOSE] Cleaning up.\n");

		delete world;
		noise_utils::cleanUpNoise();

		logging::printf(logging::RS_CRITICAL, "------------------------------------------------\n");
		logging::printf(logging::RS_CRITICAL, "Simulation completed successfully...\n\n");

		return 0;
	}
	catch (std::exception& ex)
	{
		logging::printf(logging::RS_CRITICAL,
		                "[ERROR] Simulation encountered unexpected error:\n\t%s\nSimulator will terminate.\n",
		                ex.what());
		return 1;
	}
}
