// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

/**
 * @file main.cpp
 * @brief Entry point for the FERS command-line interface (CLI).
 *
 * This executable acts as a wrapper around the libfers core library. It parses
 * command-line arguments, uses the libfers C-API to load and run a simulation,
 * and reports progress to the console.
 */

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <libfers/api.h>
#include <libfers/logging.h>
#include <libfers/parameters.h>
#include "arg_parser.h"

using logging::Level;

/**
 * @brief Reads the entire content of a file into a string.
 * @param filename The path to the file.
 * @return A string with the file content, or an empty string on failure.
 */
std::string readFileToString(const std::string& filename)
{
	std::ifstream file(filename);
	if (!file)
	{
		LOG(Level::FATAL, "Failed to open script file: {}", filename);
		return "";
	}
	return {(std::istreambuf_iterator(file)), std::istreambuf_iterator<char>()};
}

int main(const int argc, char* argv[])
{
	// Parse command-line arguments using the local arg parser
	const auto config_result = core::parseArguments(argc, argv);
	if (!config_result)
	{
		if (config_result.error() != "Help requested." && config_result.error() != "Version requested." &&
			config_result.error() != "No arguments provided.")
		{
			LOG(Level::ERROR, "Argument parsing error: {}", config_result.error());
			return 1;
		}
		return 0;
	}

	const auto& [script_file, log_level, num_threads, validate, log_file, generate_kml] = config_result.value();

	// Configure logging
	logging::logger.setLevel(log_level);
	if (log_file)
	{
		if (const auto result = logging::logger.logToFile(*log_file); !result)
		{
			LOG(Level::ERROR, "Failed to open log file: {}", result.error());
			return 1;
		}
	}

	LOG(Level::INFO, "FERS CLI started. Using libfers backend.");

	LOG(Level::DEBUG,
		"Running FERS with arguments: script_file={}, log_level={}, num_threads={}, validate={}, log_file={}",
		script_file, logging::getLevelString(log_level), num_threads, validate, log_file.value_or("None"));

	// Create a simulation context using the C-API
	fers_context_t* context = fers_context_create();
	if (!context)
	{
		LOG(Level::FATAL, "Failed to create FERS simulation context.");
		return 1;
	}

	// Load the scenario from file via the C-API
	LOG(Level::INFO, "Loading scenario from '{}'...", script_file);
	if (fers_load_scenario_from_xml_file(context, script_file.c_str(), validate) != 0)
	{
		LOG(Level::FATAL, "Failed to load scenario. Check logs for parsing errors.");
		fers_context_destroy(context);
		return 1;
	}

	if (generate_kml)
	{
		std::filesystem::path kml_output_path = script_file;
		kml_output_path.replace_extension(".kml");
		const std::string kml_output_file = kml_output_path.string();

		LOG(Level::INFO, "Generating KML file for scenario: {}", kml_output_file);
		if (fers_generate_kml(context, kml_output_file.c_str()) == 0)
		{
			LOG(Level::INFO, "KML file generated successfully: {}", kml_output_file);
		}
		else
		{
			LOG(Level::FATAL, "Failed to generate KML file.");
		}

		fers_context_destroy(context);
		return 0; // Exit after generating KML
	}

	// Set thread count via the parameters module
	if (const auto result = params::setThreads(num_threads); !result)
	{
		LOG(Level::ERROR, "Failed to set number of threads: {}", result.error());
	}

	// Run the simulation via the C-API, providing the console callback
	LOG(Level::INFO, "Starting simulation...");
	if (fers_run_simulation(context, nullptr, nullptr) != 0)
	{
		LOG(Level::FATAL, "Simulation run failed.");
		fers_context_destroy(context);
		return 1;
	}
	LOG(Level::INFO, "Simulation completed successfully.");

	fers_context_destroy(context);

	return 0;
}
