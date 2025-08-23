/**
* @file main.cpp
* @brief Entry point and main logic for the FERS simulation application.
*
* This file contains the main function that initializes and runs the FERS
* (Flexible Extensible Radar Simulator) simulation.
* It handles command-line argument parsing, logging configuration,
* simulation initialization, and execution using multithreading.
* This is the central component of the simulation framework.
*
* @authors David Young, Marc Brooker
* @date 2006-04-25
*/

#include <exception>
#include <memory>
#include <optional>

#include "core/arg_parser.h"
#include "core/logging.h"
#include "core/parameters.h"
#include "core/sim_threading.h"
#include "core/thread_pool.h"
#include "core/world.h"
#include "serial/kml_generator.h"
#include "serial/xml_parser.h"

using logging::Level;

/**
 * @brief Entry point for the FERS simulation application.
 *
 * This function initializes the simulation environment, parses command-line arguments,
 * sets up logging based on user input, and runs the simulation using a multithreaded approach.
 * It manages the execution of the simulation and handles errors encountered during the initialization
 * or simulation stages.
 *
 * @param argc The number of command-line arguments passed to the program.
 * @param argv The array of command-line arguments passed to the program.
 * @return int Returns 0 on successful simulation execution, or 1 on error.
 */
int main(const int argc, char* argv[])
{
	// Parse arguments
	const auto config_result = core::parseArguments(argc, argv);
	if (!config_result) // Check if there is an error
	{
		// Log or display the error message
		if (config_result.error() != "Help requested." && config_result.error() != "Version requested." &&
			config_result.error() != "No arguments provided.")
		{
			LOG(Level::ERROR, "Argument parsing error: {}", config_result.error());
			return 1;
		}
		return 0; // Exit if help, version, or no arguments provided
	}

	// Structured bindings for the configuration options
	const auto& [script_file, log_level, num_threads, validate, log_file, kml_output_file] = config_result.value();

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
		// Create the world object
		const auto world = std::make_unique<core::World>();

		// Load the XML file and deserialize it into the world object
		try { serial::parseSimulation(script_file, world.get(), validate); }
		catch (const std::exception&)
		{
			LOG(Level::FATAL, "Failed to load simulation file: {}", script_file);
			return 1;
		}

		// If KML generation is requested, generate the KML file and exit
		if (kml_output_file)
		{
			LOG(Level::INFO, "Generating KML file for scenario: {}", script_file);
			if (serial::KmlGenerator::generateKml(*world, *kml_output_file))
			{
				LOG(Level::INFO, "KML file generated successfully: {}", *kml_output_file);
				return 0;
			}
			LOG(Level::FATAL, "Failed to generate KML file for scenario: {}", script_file);
			return 1;
		}

		// Set the number of threads to use for the simulation
		if (const auto result = params::setThreads(num_threads); !result)
		{
			LOG(Level::ERROR, "Failed to set number of threads: {}", result.error());
		}

		// Run the simulation using the threading mechanism
		pool::ThreadPool pool(params::renderThreads());

		if (params::isCwSimulation())
		{
			core::runThreadedCwSim(world.get(), pool);
		}
		else
		{
			runThreadedSim(world.get(), pool);
		}

		LOG(Level::INFO, "Simulation completed successfully!");

		return 0;
	}
	catch (const std::exception& ex)
	{
		// Catch-all for std::exception errors
		LOG(Level::FATAL, "Simulation encountered unexpected error: {}\nSimulator will terminate.", ex.what());
		return 1;
	}
	catch (...)
	{
		// Catch-all for any non-std::exception errors
		LOG(Level::FATAL, "An unknown error occurred.\nSimulator will terminate.");
		return 1;
	}
}
