// main.cpp
// Contains the main function and some support code for FERS
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 25 April 2006

#include <exception>
#include <memory>
#include <optional>

#include "core/arg_parser.h"
#include "core/logging.h"
#include "core/parameters.h"
#include "core/sim_threading.h"
#include "core/world.h"
#include "serial/xml_parser.h"

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
	const auto config_result = core::parseArguments(argc, argv);
	if (!config_result) // Check if there is an error
	{
		// Log or display the error message
		if (config_result.error() != "Help requested." && config_result.error() != "Version requested.")
		{
			LOG(Level::ERROR, "Argument parsing error: {}", config_result.error());
		}
		return 1; // Invalid arguments or help/version shown
	}

	// Structured bindings for the configuration options
	const auto& [script_file, log_level, num_threads, validate, log_file] = config_result.value();

	// Set the logging level
	logging::logger.setLevel(log_level);

	// Check if the log file was specified and set logging to file
	if (log_file)
	{
		if (const auto result = logging::logger.logToFile(*log_file); !result)
		{
			LOG(Level::ERROR, "Failed to open log file: {}", result.error());
			return 1;
		}
	}

	LOG(Level::DEBUG,
	    "Running FERS with arguments: script_file={}, log_level={}, num_threads={}, validate={}, log_file={}",
	    script_file, logging::getLevelString(log_level), num_threads, validate, log_file.value_or("None"));

	try
	{
		// Set the number of threads to use for the simulation
		if (const auto result = params::setThreads(num_threads); !result)
		{
			LOG(Level::ERROR, "Failed to set number of threads: {}", result.error());
		}

		// Create the world object
		const auto world = std::make_unique<core::World>();

		// Load the XML file and deserialize it into the world object
		try { serial::parseSimulation(script_file, world.get(), validate); }
		catch (const std::exception&)
		{
			LOG(Level::FATAL, "Failed to load simulation file: {}", script_file);
			return 1;
		}

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
