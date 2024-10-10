/**
 * @file arg_parser.h
 * @brief Command-line argument parsing utilities for the application.
 *
 * This header file provides utilities for parsing command-line arguments,
 * displaying help, version information, and configuring the application via a configuration structure.
 * It supports validation, multi-threading settings, and logging configuration.
 * The `Config` structure holds all relevant settings,
 * while the helper functions assist in managing and displaying information to the user.
 *
 * @author David Young
 * @date 2024-09-20
 */

#pragma once

#include <expected>
#include <optional>
#include <string>

#include "logging.h"
#include "portable_utils.h"

namespace core
{
	/**
	 * @class Config
	 * @brief Configuration structure for the application.
	 *
	 * This structure holds the configuration settings parsed from the command-line arguments.
	 * It includes options such as the script file path, logging level,
	 * number of threads, input validation, and optional log file path.
	 *
	 * Example usage:
	 * @code
	 * core::Config config;
	 * config.script_file = "sim1.fersxml";
	 * config.log_level = logging::Level::DEBUG;
	 * config.num_threads = 4;
	 * config.validate = true;
	 * config.log_file = "log.txt";
	 * @endcode
	 */
	struct Config
	{
		std::string script_file; ///< Path to the script file.
		logging::Level log_level = logging::Level::INFO; ///< Logging level.
		unsigned num_threads = countProcessors(); ///< Number of threads to use, defaults to the number of processors.
		bool validate = false; ///< Validate the input .fersxml file.
		std::optional<std::string> log_file; ///< Optional log file path for logging output.
	};

	/**
	 * @brief Displays the help message.
	 *
	 * Outputs detailed help information describing the usage of the program, available options, and arguments.
	 * This function is invoked when the user requests help through command-line flags.
	 *
	 * @param programName The name of the program.
	 */
	void showHelp(const char* programName) noexcept;

	/**
	 * @brief Displays the version information.
	 *
	 * Outputs the current version and author information of the application.
	 * This function is invoked when the user requests the version via command-line flags.
	 */
	void showVersion() noexcept;

	/**
	 * @brief Parses command-line arguments.
	 *
	 * Processes the command-line arguments, validating them and extracting
	 * configurations like script file, logging level, and thread count.
	 * If an error occurs (e.g., invalid log level or missing script file),
	 * the function returns an error message.
	 *
	 * @param argc The argument count.
	 * @param argv The argument vector.
	 * @return std::expected<Config, std::string> Parsed configuration or an error message.
	 */
	std::expected<Config, std::string> parseArguments(int argc, char* argv[]) noexcept;
}
