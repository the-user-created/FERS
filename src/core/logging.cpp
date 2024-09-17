// logging.cpp
// Message support functions and debug levels
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 20 March 2006

#include "logging.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <sstream>

namespace logging
{
	Level debug_level = RS_VERY_VERBOSE;
	std::mutex debug_mutex;

	void print(const Level level, const std::string& str, const std::string& file, const int line)
	{
		if (level >= debug_level)
		{
			std::lock_guard lock(debug_mutex);
			std::ostringstream oss;
			oss << "[" << file << " " << line << "] " << str << "\n";
			std::cerr << oss.str();
		}
	}

	void printf(const Level level, const char* format, ...)
	{
		if (level >= debug_level)
		{
			std::lock_guard lock(debug_mutex);
			va_list ap;
			va_start(ap, format);
			vfprintf(stderr, format, ap);
			va_end(ap);
		}
	}

	void setDebugLevel(const Level level)
	{
		if (level <= RS_EXTREMELY_CRITICAL) { debug_level = level; }
	}
}
