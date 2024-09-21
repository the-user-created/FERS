// main.cpp
// Contains the main function and some support code for FERS
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 25 April 2006

#include <memory>

#include "arg_parser.h"
#include "parameters.h"
#include "sim_threading.h"
#include "world.h"
#include "noise/noise_utils.h"
#include "serialization/xmlimport.h"

using logging::Level;

int main(const int argc, char* argv[])
{
	const auto config_opt = core::parseArguments(argc, argv);
	if (!config_opt)
	{
		return 1; // Invalid arguments or help/version shown
	}

	const auto& [script_file, log_level, num_threads] = *config_opt;

	// Set the logging level
	logging::logger.setLevel(log_level);

	try
	{
		params::setThreads(num_threads);
		const auto world = std::make_unique<core::World>();
		noise::initializeNoise();

		serial::loadXmlFile(script_file, world.get());

		runThreadedSim(params::renderThreads(), world.get());

		LOG(Level::INFO, "Cleaning up");

		noise::cleanUpNoise();

		LOG(Level::INFO, "Simulation completed successfully!");

		return 0;
	}
	catch (std::exception& ex)
	{
		LOG(Level::FATAL, "Simulation encountered unexpected error:\t{}\nSimulator will terminate.",
		    ex.what());
		return 1;
	}
}
