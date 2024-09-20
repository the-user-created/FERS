//
// Created by davidyoung on 9/20/24.
//

#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include "logging.h"
#include "portable_utils.h"

namespace arg_parser
{
	struct Config
	{
		std::string script_file;
		logging::Level log_level = logging::Level::INFO;
		unsigned num_threads = portable_utils::countProcessors();
	};

	void showHelp(const char* programName);

	void showVersion();

	std::optional<Config> parseArguments(int argc, char* argv[]);
}

#endif //ARG_PARSER_H
