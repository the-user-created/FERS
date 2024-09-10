//rsportable.h
//Declarations for functions containing non-standard code
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//29 May 2006

#ifndef RS_PORTABLE_H
#define RS_PORTABLE_H

#include <config.h>

namespace rsPortable
{
	/// Compare two strings, ignoring case
	int stricmp(const char* one, const char* two);

	/// Compute the first order Bessel function of the first kind
	rsFloat BesselJ1(rsFloat x);

	/// Floating point round
	rsFloat rsRound(rsFloat x);

	/// Detect the number of CPUs in the machine
	int CountProcessors();
}

#endif
