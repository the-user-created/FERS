//rsportable.cpp
//All code which is non-standard C++ must go in here, with reasons why
//There should be very little code in this file
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//26 July 2006

#include "rsportable.h"

#include <cmath>
#include <cstdio>
#include <boost/thread.hpp>

#include "rsdebug.h"

/// Compare two strings, ignoring case
//There isn't a suitable one in the standard library of C or C++
int rs_portable::stricmp(const char* one, const char* two)
{
	return strcasecmp(one, two); //strcasecmp is a GNU extension
}

/// Compute the first order Bessel function of the first kind
RS_FLOAT rs_portable::besselJ1(const RS_FLOAT x)
{
	return j1(x); //j1 is non standard, but found on many platforms
}

/// Round off a floating point number
//This function isn't in C++03, but is in C++0x and TR1 and C99, so should work on most machines
RS_FLOAT rs_portable::rsRound(const RS_FLOAT x)
{
	return round(x);
}

/// Detect the number of CPUs in the machine
int rs_portable::countProcessors()
{
	//C Tong: This can now be done with boost:
	// David Young: This can be done with std::thread::hardware_concurrency() in C++11
	// TODO: Update to use std::thread::hardware_concurrency()
	if (const int i_n_hardware_threads = boost::thread::hardware_concurrency(); !i_n_hardware_threads)
	{
		rs_debug::printf(rs_debug::RS_IMPORTANT, "[IMPORTANT] Unable to get CPU count, assuming 1.\n");
		return 1;
	}
	else
	{
		return i_n_hardware_threads;
	}
}

