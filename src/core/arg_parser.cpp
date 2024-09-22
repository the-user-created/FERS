// arg_parser.cpp
// Created by David Young on 9/20/24.
//

#include "arg_parser.h"

#include <exception>              // for exception
#include <iostream>               // for operator<<, basic_ostream, cerr, cout
#include <stdexcept>              // for invalid_argument
#include <unordered_map>          // for unordered_map, operator==, _Node_it...
#include <utility>                // for pair

#include "core/logging.h"         // for Level, log, LOG
#include "core/portable_utils.h"  // for countProcessors

using logging::Level;

namespace core
{
	/**
	 * @brief Displays the help message.
	 *
	 * @param programName The name of the program.
	 */
	void showHelp(const char* programName)
	{
		std::cout << R"(/------------------------------------------------\
| FERS - The Flexible Extensible Radar Simulator |
| Version 0.28                                   |
\------------------------------------------------/
Usage: )" << programName << R"( <scriptfile> [options]

Options:
  --help, -h              Show this help message and exit
  --version, -v           Show version information and exit
  --log-level=<level>     Set the logging level (TRACE, DEBUG, INFO, WARNING, ERROR, FATAL)
  -n=<threads>            Number of threads to use

Arguments:
  <scriptfile>            Path to the simulation script file (XML)

Example:
  )" << programName << R"( simulation.xml --log-level=DEBUG -n=4

This program runs radar simulations based on an XML script file.
Make sure the script file follows the correct format to avoid errors.
)";
	}

	/**
	 * @brief Displays the version information.
	 */
	void showVersion()
	{
		std::cout << "FERS - The Flexible Extensible Radar Simulator\n";
		std::cout << "Version 0.28\n";
		std::cout << "Author: Marc Brooker\n";
	}

	/**
	 * @brief Parses the logging level from a string.
	 *
	 * @param level The logging level as a string.
	 * @return std::optional<Level> The corresponding logging level, or std::nullopt if invalid.
	 */
	std::optional<Level> parseLogLevel(const std::string& level)
	{
		static const std::unordered_map<std::string, Level> LEVEL_MAP = {
			{"TRACE", Level::TRACE},
			{"DEBUG", Level::DEBUG},
			{"INFO", Level::INFO},
			{"WARNING", Level::WARNING},
			{"ERROR", Level::ERROR},
			{"FATAL", Level::FATAL}
		};

		if (const auto it = LEVEL_MAP.find(level); it != LEVEL_MAP.end()) { return it->second; }
		return std::nullopt;
	}

	/**
	 * @brief Parses command-line arguments.
	 *
	 * @param argc Argument count.
	 * @param argv Argument vector.
	 * @return std::optional<Config> Parsed configuration or std::nullopt if parsing failed.
	 */
	std::optional<Config> parseArguments(const int argc, char* argv[])
	{
		Config config;
		bool script_file_set = false;

		if (argc < 2)
		{
			showHelp(argv[0]);
			return std::nullopt;
		}

		for (int i = 1; i < argc; ++i)
		{
			if (std::string arg = argv[i]; arg == "--help" || arg == "-h")
			{
				showHelp(argv[0]);
				return std::nullopt;
			}
			else
			{
				if (arg == "--version" || arg == "-v")
				{
					showVersion();
					return std::nullopt;
				}
				if (arg.rfind("--log-level=", 0) == 0)
				{
					// starts with --log-level=
					std::string level_str = arg.substr(12);
					if (auto level = parseLogLevel(level_str)) { config.log_level = *level; }
					else
					{
						std::cerr << "Error: Invalid log level '" << level_str << "'\n";
						return std::nullopt;
					}
				}
				else if (arg.rfind("-n=", 0) == 0)
				{
					// starts with -n=
					try
					{
						config.num_threads = std::stoi(arg.substr(3));
						if (config.num_threads <= 0)
						{
							throw std::invalid_argument("Number of threads must be greater than 0");
						}

						if (const unsigned max_threads = countProcessors(); config.num_threads >
							max_threads)
						{
							LOG(Level::ERROR,
							    "Number of threads specified is greater than the number of processors. Defaulting to the number of processors.");
							config.num_threads = max_threads;
						}
					}
					catch (const std::exception&)
					{
						std::cerr << "Error: Invalid number of threads specified\n";
						return std::nullopt;
					}
				}
				else if (arg[0] != '-' && !script_file_set)
				{
					// Assume it's the script file if it's not an option
					config.script_file = arg;
					script_file_set = true;
				}
				else
				{
					std::cerr << "Error: Unrecognized option or argument '" << arg << "'\n";
					return std::nullopt;
				}
			}
		}

		if (!script_file_set)
		{
			std::cerr << "Error: No script file provided\n";
			return std::nullopt;
		}

		return config;
	}
}
