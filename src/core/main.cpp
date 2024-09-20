// main.cpp
// Contains the main function and some support code for FERS
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 25 April 2006

#include <iostream>

#include "arg_parser.h"
#include "logging.h"
#include "parameters.h"
#include "portable_utils.h"
#include "noise/noise_generators.h"
#include "noise/noise_utils.h"
#include "serialization/xmlimport.h"
#include "simulation/sim_threading.h"

int main(const int argc, char* argv[])
{
	const auto config_opt = arg_parser::parseArguments(argc, argv);
	if (!config_opt)
	{
		return 1; // Invalid arguments or help/version shown
	}

	const auto& [script_file, log_level, num_threads] = *config_opt;

	// Set the logging level
	logging::logger.setLevel(log_level);

	try
	{
		parameters::setThreads(num_threads);
		const auto world = std::make_unique<rs::World>();
		noise_utils::initializeNoise();

		xml::loadXmlFile(script_file, world.get());

		rs::threaded_sim::runThreadedSim(parameters::renderThreads(), world.get());

		LOG(logging::Level::INFO, "Cleaning up");

		noise_utils::cleanUpNoise();

		LOG(logging::Level::INFO, "Simulation completed successfully!");

		return 0;
	}
	catch (std::exception& ex)
	{
		LOG(logging::Level::FATAL, "Simulation encountered unexpected error:\t{}\nSimulator will terminate.",
		    ex.what());
		return 1;
	}
}
