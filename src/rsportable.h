//rsportable.h
//Declarations for functions containing non-standard code
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//29 May 2006

#ifndef RS_PORTABLE_H
#define RS_PORTABLE_H

#include "config.h"

namespace rs_portable
{
	/// Compare two strings, ignoring case
	int stricmp(const char* one, const char* two);

	/// Compute the first order Bessel function of the first kind
	RS_FLOAT besselJ1(RS_FLOAT x);

	/// Floating point round
	RS_FLOAT rsRound(RS_FLOAT x);

	/// Detect the number of CPUs in the machine
	unsigned int countProcessors();
}

#endif
