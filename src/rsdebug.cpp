//Implementation of rsdebug.h - Debug messaging
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//20 March 2006

#include "rsdebug.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <boost/thread/mutex.hpp> //for boost::mutex

rs_debug::Level debug_level = rs_debug::RS_VERY_VERBOSE; //The current debug level

//We use a mutex for log information printing, to stop messages from getting mangled
boost::mutex debug_mutex;

//Print out a debug message including the line and file name
void rs_debug::print(const Level level, const std::string& str, const std::string& file, const int line)
{
	if (level >= debug_level)
	{
		boost::mutex::scoped_lock lock(debug_mutex); //Lock the mutex
		std::ostringstream oss;
		oss << "[" << file << " " << line << "] ";
		oss << str << "\n";
		std::cerr << oss.str();
		//Mutex will automatically be unlocked here by scoped_lock
	}
}

//Formatted print of the current debug level, doesn't include filename and line
//Uses the cstdarg variable arguments system and the vfprintf function to handle the arguments
//If your system does not have the standard vfprintf function in it's library, you will have to make a pla
void rs_debug::printf(const Level level, const char* format, ...)
{
	if (level >= debug_level)
	{
		boost::mutex::scoped_lock lock(debug_mutex); //Lock the mutex
		va_list ap;
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
		//mutex will automatically be unlocked here by scoped_lock
	}
}

//See comments for printf(Level, char *)
void rs_debug::printf(const Level level, const std::string& format, ...)
{
	if (level >= debug_level)
	{
		boost::mutex::scoped_lock lock(debug_mutex); //Lock the mutex
		va_list ap;
		va_start(ap, format);
		vfprintf(stderr, format.c_str(), ap);
		va_end(ap);
		//mutex will automatically be unlocked here by scoped_lock
	}
}

//Set the current debug level
void rs_debug::setDebugLevel(const Level level)
{
	if (level <= RS_EXTREMELY_CRITICAL)
	{
		debug_level = level;
	}
}


