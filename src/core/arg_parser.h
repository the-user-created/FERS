// arg_parser.h
// Created by David Young on 9/20/24.
//

#pragma once

#include <optional>
#include <string>

#include "logging.h"
#include "portable_utils.h"

namespace core
{
	/**
	 * @brief Configuration structure for the application.
	 */
	struct Config
	{
		std::string script_file; ///< Path to the script file.
		logging::Level log_level = logging::Level::INFO; ///< Logging level.
		unsigned num_threads = countProcessors(); ///< Number of threads to use, defaults to the number of processors.
	};

	/**
	 * @brief Displays the help message.
	 *
	 * @param programName The name of the program.
	 */
	void showHelp(const char* programName);

	/**
	 * @brief Displays the version information.
	 */
	void showVersion();

	/**
	 * @brief Parses command-line arguments.
	 *
	 * @param argc Argument count.
	 * @param argv Argument vector.
	 * @return std::optional<Config> Parsed configuration or std::nullopt if parsing failed.
	 */
	std::optional<Config> parseArguments(int argc, char* argv[]);
}
