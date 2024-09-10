//rsthreadedsim.h
//Definitions for threaded simulation code
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//19 July 2006

#ifndef RS_THREADEDSIM_H
#define RS_THREADEDSIM_H

#include "rsworld.h"

namespace rs
{
	void runThreadedSim(int threadLimit, World* world);
}

#endif
