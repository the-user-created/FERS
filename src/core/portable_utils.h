// portable_utils.h
// Declarations for functions containing non-standard code
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 29 May 2006

#ifndef PORTABLE_UTILS_H
#define PORTABLE_UTILS_H

#include <cmath>
#include <strings.h>
#include <thread>

#include "config.h"
#include "logging.h"

namespace portable_utils
{
	// strcasecmp is a GNU extension
	inline int stricmp(const char* one, const char* two)
	{
		// Note: This function is not used in the codebase
		return strcasecmp(one, two);
	}

	// j1 is non-standard, but found on many platforms
	inline RS_FLOAT besselJ1(const RS_FLOAT x) // TODO: only used in antenna_factory.cpp
	{
		return j1(x);
	}

	// TODO: Should this be removed since it was a compatibility function for C++03?
	inline RS_FLOAT rsRound(const RS_FLOAT x)
	{
		return round(x);
	}

	// Detect the number of CPUs in the machine
	inline unsigned countProcessors()  // TODO: only used in main.cpp
	{
		const unsigned hardware_threads = std::thread::hardware_concurrency();
		return hardware_threads
			       ? hardware_threads
			       : (logging::printf(logging::RS_IMPORTANT, "[IMPORTANT] Unable to get CPU count, assuming 1.\n"), 1);
	}
}

#endif
