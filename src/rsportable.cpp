//rsportable.cpp
//All code which is non-standard C++ must go in here, with reasons why
//There should be very little code in this file
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//26 July 2006

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <sstream>
#include "rsportable.h"
#include "rsdebug.h"

/// Compare two strings, ignoring case
//There isn't a suitable one in the standard library of C or C++
int rsPortable::stricmp(const char *one, const char *two)
{
  return strcasecmp(one, two); //strcasecmp is a GNU extension
}

/// Compute the first order Bessel function of the first kind
rsFloat rsPortable::BesselJ1(rsFloat x)
{
    return j1(x); //j1 is non standard, but found on many platforms
}

/// Round off a floating point number
//This function isn't in C++03, but is in C++0x and TR1 and C99, so should work on most machines
rsFloat rsPortable::rsRound(rsFloat x) 
{
  return round(x);
}

/// Detect the number of CPUs in the machine
int rsPortable::CountProcessors()
{
  // TODO: Do this in a non Linux specific way
  FILE *fp;
  // Get the CPU count from /proc/cpuinfo
  fp = popen("cat /proc/cpuinfo | grep \"processor\" | wc -l", "r");
  // If that is not possible, assume 1 CPU
  if (!fp) {
    rsDebug::printf(rsDebug::RS_IMPORTANT, "[IMPORTANT] Unable to get CPU count, assuming 1.\n");
    return 1;
  }
  // Parse the output
  char buffer[32]; // Probably will never have more than 10^30 processors
  if (fgets(buffer, 32, fp)) {
    std::istringstream iss(buffer);
    int count;
    iss >> count; // Parse the output to get an integer
    int use_count = count;
    // Set sensible limits on CPU count
    if (use_count < 1)
      use_count = 1;
    if (use_count > 32) // A sensible upper limit for now, might need to change sometime next year
      use_count = 32;
    rsDebug::printf(rsDebug::RS_IMPORTANT, "[IMPORTANT] Got CPU count %d. Using %d threads.\n", count, use_count);
    pclose(fp);
    return use_count;
  }
  pclose(fp);
  rsDebug::printf(rsDebug::RS_IMPORTANT, "[IMPORTANT] Unable to get CPU count, assuming 1.\n");
  return 1;
}

