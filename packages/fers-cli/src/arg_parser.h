// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2024-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
* @file arg_parser.h
* @brief Command-line argument parsing utilities for the application.
*
* This header file provides utilities for parsing command-line arguments,
* displaying help, version information, and configuring the application via a configuration structure.
*/

#pragma once

#include <expected>
#include <optional>
#include <string>

#include <libfers/logging.h>
#include <libfers/portable_utils.h>

namespace core
{
	/**
	 * @class Config
	 * @brief Configuration structure for the application.
	 */
	struct Config
	{
		std::string script_file; ///< Path to the script file.
		logging::Level log_level = logging::Level::INFO; ///< Logging level.
		unsigned num_threads = countProcessors(); ///< Number of threads to use, defaults to the number of processors.
		bool validate = true; ///< Validate the input .fersxml file by default.
		std::optional<std::string> log_file; ///< Optional log file path for logging output.
		bool generate_kml = false; ///< Optional flag to generate KML visualization output.
	};

	/**
	 * @brief Displays the help message.
	 *
	 * @param programName The name of the program.
	 */
	void showHelp(const char* programName) noexcept;

	/**
	 * @brief Displays the version information.
	 */
	void showVersion() noexcept;

	/**
	 * @brief Parses command-line arguments.
	 *
	 * Processes the command-line arguments, validating them and extracting
	 * configurations like script file, logging level, and thread count.
	 *
	 * @param argc The argument count.
	 * @param argv The argument vector.
	 * @return std::expected<Config, std::string> Parsed configuration or an error message.
	 */
	std::expected<Config, std::string> parseArguments(int argc, char* argv[]) noexcept;
}
