//rsworld.cpp
//Implementation of simulator world object
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 25 April 2006

#include "rsworld.h"

#include <algorithm>

#include "rsantenna.h"
#include "rsmultipath.h"
#include "rsplatform.h"
#include "rsradar.h"
#include "rsradarwaveform.h"
#include "rstarget.h"
#include "rstiming.h"

using namespace rs; //Import the rs namespace for this implementation

/// Function object used to delete objects from vector
template <typename T>
struct ObjDel
{
	//operator() deletes the object that x is a pointer to
	void operator()(T x)
	{
		delete x;
	}
};


//Default constructor
World::World():
	_multipath_surface(nullptr)
{
}

//World object destructor
World::~World()
{
	//Delete all the objects in the world
	for (std::map<std::string, RadarSignal*>::iterator iter = _pulses.begin(); iter != _pulses.end(); ++iter)
	{
		delete (*iter).second;
	}

	for (std::map<std::string, Antenna*>::iterator aiter = _antennas.begin(); aiter != _antennas.end(); ++aiter)
	{
		delete (*aiter).second;
	}

	for (std::map<std::string, PrototypeTiming*>::iterator titer = _timings.begin(); titer != _timings.end(); ++titer)
	{
		delete (*titer).second;
	}

	std::for_each(_receivers.begin(), _receivers.end(), ObjDel<Receiver*>());
	std::for_each(_transmitters.begin(), _transmitters.end(), ObjDel<Transmitter*>());
	std::for_each(_targets.begin(), _targets.end(), ObjDel<Target*>());

	//Platforms are deleted last as they are referred to by the other object types
	std::for_each(_platforms.begin(), _platforms.end(), ObjDel<Platform*>());
}

//Add a platform to the world
void World::add(Platform* plat)
{
	_platforms.push_back(plat);
}

//Add a transmitter to the world
void World::add(Transmitter* trans)
{
	_transmitters.push_back(trans);
}

//Add a receiver to the world
void World::add(Receiver* recv)
{
	_receivers.push_back(recv);
}

//Add a target to the world
void World::add(Target* target)
{
	_targets.push_back(target);
}

//Add a pulse to the world
void World::add(RadarSignal* pulse)
{
	if (findSignal(pulse->getName()))
	{
		throw std::runtime_error(
			"[ERROR] A pulse with the name " + pulse->getName() + " already exists. Pulses must have unique names");
	}
	_pulses[pulse->getName()] = pulse;
}

//Add an antenna to the world
void World::add(Antenna* antenna)
{
	if (findAntenna(antenna->getName()))
	{
		throw std::runtime_error(
			"[ERROR] An antenna with the name " + antenna->getName() +
			" already exists. Antennas must have unique names");
	}
	_antennas[antenna->getName()] = antenna;
}

//Add a timing source to the world
void World::add(PrototypeTiming* timing)
{
	if (findTiming(timing->getName()))
	{
		throw std::runtime_error(
			"[ERROR] A timing source with the name " + timing->getName() +
			" already exists. Timing sources must have unique names");
	}
	_timings[timing->getName()] = timing;
}

//Get a pulse from the map of pulses
RadarSignal* World::findSignal(const std::string& name)
{
	return _pulses[name];
}

//Get an antenna from the map of antennas
Antenna* World::findAntenna(const std::string& name)
{
	return _antennas[name];
}

/// Find a timing source with the specified name
PrototypeTiming* World::findTiming(const std::string& name)
{
	return _timings[name];
}

///Add a multipath surface to the world
void World::addMultipathSurface(MultipathSurface* surface)
{
	if (_multipath_surface)
	{
		throw std::runtime_error("[ERROR] Only one multipath surface per simulation is supported");
	}
	_multipath_surface = surface;
}

///Process the scene to add virtual receivers and transmitters
void World::processMultipath()
{
	// In this function "duals" are added for each transmitter and receiver
	// a dual has the same properties of the transmitter and receiver, but is reflected in the multipath plane
	if (_multipath_surface)
	{
		//Add duals for each plaform
		std::vector<Platform*>::iterator plat = _platforms.begin();
		for (const std::vector<Platform*>::iterator plat_end = _platforms.end(); plat != plat_end; ++plat)
		{
			_platforms.push_back(createMultipathDual(*plat, _multipath_surface));
		}
		//Add duals for each receiver
		std::vector<Receiver*>::iterator recv = _receivers.begin();
		for (const std::vector<Receiver*>::iterator recv_end = _receivers.end(); recv != recv_end; ++recv)
		{
			_receivers.push_back(createMultipathDual(*recv, _multipath_surface));
		}
		//Add duals for each transmitter
		std::vector<Transmitter*>::iterator trans = _transmitters.begin();
		for (const std::vector<Transmitter*>::iterator trans_end = _transmitters.end(); trans != trans_end; ++trans)
		{
			_transmitters.push_back(createMultipathDual(*trans, _multipath_surface));
		}
	}
	//Clean up the multipath surface
	delete _multipath_surface;
	_multipath_surface = nullptr;
}
