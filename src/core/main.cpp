// main.cpp
// Contains the main function and some support code for FERS
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 25 April 2006

#include <cstring>
#include <stdexcept>

#include "parameters.h"
#include "portable_utils.h"
#include "noise/noise_generators.h"
#include "noise/noise_utils.h"
#include "serialization/xmlimport.h"
#include "simulation/sim_threading.h"

int main(const int argc, char* argv[])
{
	logging::logger.setLevel(logging::Level::VV);

	LOG(logging::Level::INFO, "/------------------------------------------------\\");
	LOG(logging::Level::INFO, "| FERS - The Flexible Extensible Radar Simulator |");
	LOG(logging::Level::INFO, "| Version 0.28                                   |");
	LOG(logging::Level::INFO, "\\------------------------------------------------/");

	if (argc != 2 || !strncmp(argv[1], "--help", 6))
	{
		LOG(logging::Level::INFO, "Usage: {} <scriptfile> (Run simulation specified by script file)",
		    argv[0]);
		LOG(logging::Level::INFO, "Usage: {} --help (Show this message)", argv[0]);
		return 2;
	}

	try
	{
		parameters::setThreads(portable_utils::countProcessors());
		const auto world = std::make_unique<rs::World>();
		noise_utils::initializeNoise();

		LOG(logging::Level::VERBOSE, "Loading XML Script File.");
		xml::loadXmlFile(argv[1], world.get());

		rs::threaded_sim::runThreadedSim(parameters::renderThreads(), world.get());

		LOG(logging::Level::VERBOSE, "Cleaning up");

		noise_utils::cleanUpNoise();

		LOG(logging::Level::CRITICAL, "Simulation completed successfully!");

		return 0;
	}
	catch (std::exception& ex)
	{
		LOG(logging::Level::CRITICAL,
		    "Simulation encountered unexpected error:\t{}\nSimulator will terminate.",
		    ex.what());
		return 1;
	}
}
