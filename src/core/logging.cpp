// logging.cpp
// Message support functions and debug levels
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 20 March 2006

#include "logging.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <boost/thread/mutex.hpp>

namespace logging
{
	Level debug_level = RS_VERY_VERBOSE;
	boost::mutex debug_mutex;

	void print(const Level level, const std::string& str, const std::string& file, const int line)
	{
		if (level >= debug_level)
		{
			boost::mutex::scoped_lock lock(debug_mutex);
			std::ostringstream oss;
			oss << "[" << file << " " << line << "] " << str << "\n";
			std::cerr << oss.str();
		}
	}

	void printf(const Level level, const char* format, ...)
	{
		if (level >= debug_level)
		{
			boost::mutex::scoped_lock lock(debug_mutex);
			va_list ap;
			va_start(ap, format);
			vfprintf(stderr, format, ap);
			va_end(ap);
		}
	}

	void printf(const Level level, const std::string& format, ...)  // TODO: unused
	{
		if (level >= debug_level)
		{
			boost::mutex::scoped_lock lock(debug_mutex);
			va_list ap;
			va_start(ap, format);
			vfprintf(stderr, format.c_str(), ap);
			va_end(ap);
		}
	}

	void setDebugLevel(const Level level)  // TODO: unused
	{
		if (level <= RS_EXTREMELY_CRITICAL) { debug_level = level; }
	}
}
