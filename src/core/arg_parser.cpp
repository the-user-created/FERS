//
// Created by David Young on 9/20/24.
//

#include "arg_parser.h"

#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

namespace arg_parser
{
	void showHelp(const char* programName)
	{
		std::cout << "/------------------------------------------------\\" << std::endl;
		std::cout << "| FERS - The Flexible Extensible Radar Simulator |" << std::endl;
		std::cout << "| Version 0.28                                   |" << std::endl;
		std::cout << "\\------------------------------------------------/" << std::endl;
		std::cout << "Usage: " << programName << " <scriptfile> [options]" << std::endl;
		std::cout << "\nOptions:\n";
		std::cout << "  --help, -h              Show this help message and exit\n";
		std::cout << "  --version, -v           Show version information and exit\n";
		std::cout << "  --log-level=<level>     Set the logging level (TRACE, DEBUG, INFO, WARNING, ERROR, FATAL)\n";
		std::cout << "  -n=<threads>            Number of threads to use\n";
		std::cout << "\nArguments:\n";
		std::cout << "  <scriptfile>            Path to the simulation script file (XML)\n";
		std::cout << "\nExample:\n";
		std::cout << "  " << programName << " simulation.xml --log-level=DEBUG -n=4\n";
		std::cout << "\nThis program runs radar simulations based on an XML script file.\n";
		std::cout << "Make sure the script file follows the correct format to avoid errors.\n";
	}

	void showVersion()
	{
		std::cout << "FERS - The Flexible Extensible Radar Simulator\n";
		std::cout << "Version 0.28\n";
		std::cout << "Author: Marc Brooker\n";
	}

	std::optional<logging::Level> parseLogLevel(const std::string& level)
	{
		static const std::unordered_map<std::string, logging::Level> LEVEL_MAP = {
			{"TRACE", logging::Level::TRACE},
			{"DEBUG", logging::Level::DEBUG},
			{"INFO", logging::Level::INFO},
			{"WARNING", logging::Level::WARNING},
			{"ERROR", logging::Level::ERROR},
			{"FATAL", logging::Level::FATAL}
		};

		if (const auto it = LEVEL_MAP.find(level); it != LEVEL_MAP.end()) { return it->second; }
		return std::nullopt;
	}

	// Function to parse the command-line arguments
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

						if (const unsigned max_threads = portable_utils::countProcessors(); config.num_threads >
							max_threads)
						{
							LOG(logging::Level::ERROR,
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

	void setLogLevel(const logging::Level level)
	{
		switch (level)
		{
		case logging::Level::TRACE: logging::logger.setLevel(logging::Level::TRACE);
			break;
		case logging::Level::DEBUG: logging::logger.setLevel(logging::Level::DEBUG);
			break;
		case logging::Level::INFO: logging::logger.setLevel(logging::Level::INFO);
			break;
		case logging::Level::WARNING: logging::logger.setLevel(logging::Level::WARNING);
			break;
		case logging::Level::ERROR: logging::logger.setLevel(logging::Level::ERROR);
			break;
		case logging::Level::FATAL: logging::logger.setLevel(logging::Level::FATAL);
			break;
		}
	}
}
