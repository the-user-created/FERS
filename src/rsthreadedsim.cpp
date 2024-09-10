//rsthreadedsim.cpp
//Thread management for the simulator
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//29 May 2006

//One of the goals for FERS is to support multiple processors. This is achieved
//through multithreading. One simulation is performed in for each transmitter-
//receiver pair. A number of these simulations are run in parallel by multi-
//threading, according to the number of CPUs (or cores) the system has.

#include "rsthreadedsim.h"

#include <memory>
#include <vector>
#include <boost/version.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

#include "rsdebug.h"
#include "rsradar.h"
#include "rsworld.h"

#if BOOST_VERSION < 105000
#define TIME_UTC_ TIME_UTC
#endif

using namespace rs;

//Counter of currently running threads
volatile int threads;
boost::mutex threads_mutex; //Mutex to protect it

/// Flag to set if a thread encounters an error
volatile int error;
boost::mutex error_mutex;

/// Method to decrease the running thread count
static void decThreads()
{
	boost::mutex::scoped_lock lock(threads_mutex);
	threads--;
}

/// Method to flag if a thread experienced an error
static void setError()
{
	boost::mutex::scoped_lock lock(error_mutex);
	error = 1;
}

// TODO: This class should be in the header file
//SimThread contains a simulator thread
class SimThread
{
public:
	//Constructor
	SimThread(const Transmitter* transmitter, Receiver* receiver, const World* world):
		_trans(transmitter), _recv(receiver), _world(world)
	{
	}

	//Operator () is executed when we create the thread
	void operator()() const
	{
		rs_debug::printf(rs_debug::RS_VERBOSE,
		                "[VERBOSE] Created simulator thread for transmitter '%s' and receiver '%s' ",
		                _trans->getName().c_str(), _recv->getName().c_str());
		try
		{
			simulatePair(_trans, _recv, _world);
		}
		catch (std::exception& ex)
		{
			rs_debug::printf(rs_debug::RS_CRITICAL,
			                "[ERROR] First pass thread terminated with unexpected error:\n\t%s\nSimulator will terminate\n",
			                ex.what());
			setError();
		}
		decThreads();
	}

protected:
	//The transmitter/receiver pair to simulate
	const Transmitter* _trans;
	Receiver* _recv;
	const World* _world;
};

// TODO: This class should be in the header file
// RenderThread contains a thread which performs the second phase of the simulation
class RenderThread
{
public:
	/// Constructor
	explicit RenderThread(Receiver* recv):
		_recv(recv)
	{
	}

	/// Operator () is executed when we create the thread
	void operator()() const
	{
		rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Created render thread for receiver '%s'\n",
		                _recv->getName().c_str());
		try
		{
			_recv->render();
		}
		catch (std::exception& ex)
		{
			rs_debug::printf(rs_debug::RS_CRITICAL,
			                "[ERROR] Render thread terminated with unexpected error:\n\t%s\nSimulator will terminate\n",
			                ex.what());
			setError();
		}
		decThreads();
	}

protected:
	Receiver* _recv; //!< The Receiver to render
};


/// Sleep for the specified number of seconds
static void sleep(const int secs)
{
	//We sleep for one second here
	boost::xtime xt;
	boost::xtime_get(&xt, boost::TIME_UTC_);
	xt.sec += secs;
	boost::thread::sleep(xt);
}

//Increase the count of running threads
static void incThreads()
{
	boost::mutex::scoped_lock lock(threads_mutex);
	threads++;
}

//Run a sim thread for each of the receiver-transmitter pairs, limiting concurrent threads
void rs::runThreadedSim(const int threadLimit, World* world)
{
	std::vector<std::unique_ptr<boost::thread>> running;
	std::vector<Receiver*>::iterator ri;
	boost::thread mainthrd();
	rs_debug::printf(rs_debug::RS_INFORMATIVE, "[INFO] Using threaded simulation with %d threads.\n", threadLimit);
	//PHASE 1: Do first pass of simulator
	//Loop through the lists for transmitters and receivers
	for (ri = world->_receivers.begin(); ri != world->_receivers.end(); ++ri)
	{
		for (std::vector<Transmitter*>::const_iterator ti = world->_transmitters.begin(); ti != world->_transmitters.end(); ++ti)
		{
			incThreads();
			SimThread sim(*ti, *ri, world);
			std::unique_ptr<boost::thread> thrd = std::make_unique<boost::thread>(sim);
			//Delay until a thread is terminated, if we have reached the limit
			while (threads >= threadLimit)
			{
				boost::thread::yield();
			}
			//If a thread ended in error, abort the simulation
			if (error)
			{
				throw std::runtime_error("Thread terminated with error. Aborting simulation");
			}
			//Add the thread pointers to a vector to be freed later
			running.push_back(std::move(thrd));
		}
	}
	//Wait for all the first pass threads to finish before continuing
	while (threads)
	{
		boost::thread::yield();
		//      rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Main Thread Poll, Waiting on %d first pass threads.\n", threads);
		if (error)
		{
			throw std::runtime_error("Thread terminated with error. Aborting simulation");
		}
	}
	// Clean all the thread pointers
	running.clear();

	// Report on the number of responses added to each receiver
	for (ri = world->_receivers.begin(); ri != world->_receivers.end(); ++ri)
	{
		rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] %d responses added to receiver '%s'\n", (*ri)->countResponses(),
		                (*ri)->getName().c_str());
	}

	//PHASE 2: Do render pass of simulation
	//Loop through the lists of receivers and set each to render
	for (ri = world->_receivers.begin(); ri != world->_receivers.end(); ++ri)
	{
		incThreads();
		RenderThread sim(*ri);
		std::unique_ptr<boost::thread> thrd = std::make_unique<boost::thread>(sim);
		//Delay until a thread is terminated, if we have reached the limit
		while (threads >= threadLimit)
		{
			boost::thread::yield();
		}
		//If a thread ended in error, abort the simulation
		if (error)
		{
			throw std::runtime_error("Thread terminated with error. Aborting simulation");
		}
		//Add the thread pointers to a vector to be freed later
		running.push_back(std::move(thrd));
	}

	//Wait for all the render threads to finish
	while (threads)
	{
		boost::thread::yield();
		//rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Main Thread Poll, Waiting on %d render threads.\n", threads);
		if (error)
		{
			throw std::runtime_error("Thread terminated with error. Aborting simulation");
		}
	}
	//Clean all the thread pointers
	running.clear();
}
