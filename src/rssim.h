//rssim.h
//Declarations and definitions for rssim.cpp and rsthreadedsim.cpp

#ifndef RS_SIM_H
#define RS_SIM_H

#include <config.h>

#include "rsradar.h"
#include "rsworld.h"

namespace rs
{
	//Functions in rssim.cpp

	//Run a simulation on the specified receiver/transmitter pair
	void SimulatePair(const Transmitter* trans, Receiver* recv, const World* world);

	//Functions in rsthreadedsim.cpp

	//Run the radar simulation specified by world
	//Limit the number of concurrent threads to thread_limit
	void RunThread(int thread_limit, World* world);
}

#endif
