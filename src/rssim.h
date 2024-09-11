//rssim.h
//Declarations and definitions for rssim.cpp and rsthreadedsim.cpp

#ifndef RS_SIM_H
#define RS_SIM_H

#include "rsworld.h"

namespace rs
{
	//Run the radar simulation specified by world
	//Limit the number of concurrent threads to thread_limit
	void runThread(int threadLimit, World* world);
}

#endif
