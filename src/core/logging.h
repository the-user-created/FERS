// logging.h
// Message support functions and debug levels
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 20 March 2006

#ifndef LOGGING_H
#define LOGGING_H

#include <string>

#define DEBUG_PRINT(level, str) logging::print(level, str, __FILE__, __LINE__)

namespace logging
{
	enum Level
	{
		RS_VERY_VERBOSE,
		RS_VERBOSE,
		RS_INFORMATIVE,
		RS_IMPORTANT,
		RS_CRITICAL,
		RS_EXTREMELY_CRITICAL
	};

	void print(Level level, const std::string& str, const std::string& file, int line);

	void printf(Level level, const char* format, ...);

	void printf(Level level, const std::string& format, ...);

	void setDebugLevel(Level level);
}

#endif
