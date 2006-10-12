//rsportable.cpp
//All code which is non-standard C++ must go in here, with reasons why
//There should be very little code in this file
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//26 July 2006

#include <string.h>
#include <math.h>
#include "rsportable.h"

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
