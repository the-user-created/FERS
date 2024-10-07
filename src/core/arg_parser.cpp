// arg_parser.cpp
// Created by David Young on 9/20/24.
//

#include "arg_parser.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/logging.h"
#include "core/portable_utils.h"

using logging::Level;

namespace
{
	// Function to check if the file has a valid log extension
	bool isValidLogFileExtension(const std::string& filePath) noexcept
	{
		static const std::vector<std::string> VALID_EXTENSIONS = {".log", ".txt"};
		std::string extension = std::filesystem::path(filePath).extension().string();
		std::ranges::transform(extension, extension.begin(), ::tolower);
		return std::ranges::find(VALID_EXTENSIONS, extension) != VALID_EXTENSIONS.end();
	}

	/**
	 * @brief Parses the logging level from a string.
	 *
	 * @param level The logging level as a string.
	 * @return std::optional<Level> The corresponding logging level, or std::nullopt if invalid.
	 */
	std::optional<Level> parseLogLevel(const std::string& level) noexcept
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
}

namespace core
{
	/**
	 * @brief Displays the help message.
	 *
	 * @param programName The name of the program.
	 */
	void showHelp(const char* programName) noexcept
	{
		std::cout << R"(/------------------------------------------------\
| FERS - The Flexible Extensible Radar Simulator |
| Version 0.28                                   |
\------------------------------------------------/
Usage: )" << programName << R"( <scriptfile> [options]

Options:
  --help, -h              Show this help message and exit
  --version, -v           Show version information and exit
  --validate, -val		  Validate the input .fersxml file and run the simulation.
  --log-level=<level>     Set the logging level (TRACE, DEBUG, INFO, WARNING, ERROR, FATAL)
  --log-file=<file>       Log output to the specified .log or .txt file as well as the console.
  -n=<threads>            Number of threads to use

Arguments:
  <scriptfile>            Path to the simulation script file (XML)

Example:
  )" << programName << R"( simulation.fersxml --log-level=DEBUG --log-file=output.log -n=4

This program runs radar simulations based on an XML script file.
Make sure the script file follows the correct format to avoid errors.
)";
	}

	/**
	 * @brief Displays the version information.
	 */
	void showVersion() noexcept
	{
		std::cout << "FERS - The Flexible Extensible Radar Simulator\n";
		std::cout << "Version 0.28\n";
		std::cout << "Author: Marc Brooker\n";
	}

	/**
	 * @brief Parses command-line arguments.
	 *
	 * @param argc Argument count.
	 * @param argv Argument vector.
	 * @return std::optional<Config> Parsed configuration or std::nullopt if parsing failed.
	 */
	std::expected<Config, std::string> parseArguments(const int argc, char* argv[]) noexcept
	{
		Config config;
		bool script_file_set = false;

		if (argc < 2)
		{
			showHelp(argv[0]);
			return std::unexpected("No arguments provided.");
		}

		for (int i = 1; i < argc; ++i)
		{
			if (std::string arg = argv[i]; arg == "--help" || arg == "-h")
			{
				showHelp(argv[0]);
				return std::unexpected("Help requested.");
			}
			else
			{
				if (arg == "--version" || arg == "-v")
				{
					showVersion();
					return std::unexpected("Version requested.");
				}
				if (arg.rfind("--log-level=", 0) == 0)
				{
					std::string level_str = arg.substr(12);
					if (auto level = parseLogLevel(level_str)) { config.log_level = *level; }
					else
					{
						LOG(Level::ERROR, "Invalid log level '" + level_str + "'");
						return std::unexpected("Invalid log level: " + level_str);
					}
				}
				else if (arg.rfind("--log-file=", 0) == 0)
				{
					if (std::string log_file_path = arg.substr(11); isValidLogFileExtension(log_file_path))
					{
						config.log_file = log_file_path;
					}
					else
					{
						LOG(Level::ERROR, "Invalid log file extension. Log file must have .log or .txt extension.");
						return std::unexpected("Invalid log file extension: " + log_file_path);
					}
				}
				else if (arg.rfind("-n=", 0) == 0)
				{
					try
					{
						config.num_threads = std::stoi(arg.substr(3));
						if (config.num_threads == 0)
						{
							return std::unexpected("Number of threads must be greater than 0");
						}

						if (const unsigned max_threads = countProcessors(); config.num_threads > max_threads)
						{
							LOG(Level::WARNING,
							    "Number of threads exceeds available processors. Defaulting to max processors.");
							config.num_threads = max_threads;
						}
					}
					catch (const std::exception&) { return std::unexpected("Invalid number of threads specified."); }
				}
				else if (arg[0] != '-' && !script_file_set)
				{
					config.script_file = arg;
					script_file_set = true;
				}
				else if (arg == "--validate" || arg == "-val") { config.validate = true; }
				else
				{
					LOG(Level::ERROR, "Unrecognized option or argument: '" + arg + "'");
					return std::unexpected("Unrecognized argument: " + arg);
				}
			}
		}

		if (!script_file_set) { return std::unexpected("No script file provided."); }

		return config;
	}
}
