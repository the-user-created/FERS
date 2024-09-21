//
// Created by davidyoung on 9/20/24.
//

#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include <optional>
#include <string>

#include "logging.h"
#include "portable_utils.h"

namespace core
{
	struct Config
	{
		std::string script_file;
		logging::Level log_level = logging::Level::INFO;
		unsigned num_threads = countProcessors();
	};

	void showHelp(const char* programName);

	void showVersion();

	std::optional<Config> parseArguments(int argc, char* argv[]);
}

#endif //ARG_PARSER_H
