// main.cpp
// Contains the main function and some support code for FERS
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 25 April 2006

#include <memory>

#include "arg_parser.h"
#include "parameters.h"
#include "sim_threading.h"
#include "world.h"
#include "serialization/xmlimport.h"

using logging::Level;

/**
 * \brief Entry point for the FERS simulation application.
 *
 * This function initializes the simulation environment, parses command-line arguments,
 * sets up logging, and runs the simulation using a multithreaded approach.
 *
 * \param argc The number of command-line arguments.
 * \param argv The array of command-line arguments.
 * \return int Returns 0 on successful completion, 1 on error.
 */
int main(const int argc, char* argv[])
{
	// Parse arguments
	const auto config_opt = core::parseArguments(argc, argv);
	if (!config_opt)
	{
		return 1; // Invalid arguments or help/version shown
	}

	// Structured bindings for the configuration options
	const auto& [script_file, log_level, num_threads] = *config_opt;

	// Set the logging level
	logging::logger.setLevel(log_level);

	try
	{
		// Set the number of threads to use for the simulation
		params::setThreads(num_threads);

		// Create the world object
		const auto world = std::make_unique<core::World>();

		// Load the XML file and deserialize it into the world object
		serial::loadXmlFile(script_file, world.get());

		// Run the simulation using the threading mechanism
		runThreadedSim(params::renderThreads(), world.get());

		LOG(Level::INFO, "Simulation completed successfully!");

		return 0;
	}
	catch (const std::exception& ex)
	{
		// Catch-all for std::exception errors
		LOG(Level::FATAL, "Simulation encountered unexpected error:{}\nSimulator will terminate.",
		    ex.what());
		return 1;
	}
	catch (...)
	{
		// Catch-all for any non-std::exception errors
		LOG(Level::FATAL, "An unknown error occurred.\nSimulator will terminate.");
		return 1;
	}
}
