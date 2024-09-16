// world.cpp
// Implementation of simulator world object
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 25 April 2006

#include "world.h"

#include "math_utils/multipath_surface.h"
#include "radar/radar_system.h"
#include "radar/target.h"

using namespace rs;

template <typename T>
struct ObjDel
{
	void operator()(T x)
	{
		delete x;
	}
};

World::~World()
{
	for (auto& [_, snd] : _pulses)
	{
		delete snd;
	}
	for (auto& [_, snd] : _antennas)
	{
		delete snd;
	}
	for (auto& [_, snd] : _timings)
	{
		delete snd;
	}

	// TODO: These should all be moved to smart pointers
	std::for_each(_receivers.begin(), _receivers.end(), ObjDel<Receiver*>());
	std::for_each(_transmitters.begin(), _transmitters.end(), ObjDel<Transmitter*>());
	std::for_each(_targets.begin(), _targets.end(), ObjDel<Target*>());
	std::for_each(_platforms.begin(), _platforms.end(), ObjDel<Platform*>());
}

void World::add(RadarSignal* pulse)
{
	if (findSignal(pulse->getName()))
	{
		throw std::runtime_error("[ERROR] A pulse with the name " + pulse->getName() + " already exists.");
	}
	_pulses[pulse->getName()] = pulse;
}

void World::add(Antenna* antenna)
{
	if (findAntenna(antenna->getName()))
	{
		throw std::runtime_error("[ERROR] An antenna with the name " + antenna->getName() + " already exists.");
	}
	_antennas[antenna->getName()] = antenna;
}

void World::add(PrototypeTiming* timing)
{
	if (findTiming(timing->getName()))
	{
		throw std::runtime_error("[ERROR] A timing source with the name " + timing->getName() + " already exists.");
	}
	_timings[timing->getName()] = timing;
}

void World::addMultipathSurface(MultipathSurface* surface)
{
	if (_multipath_surface)
	{
		throw std::runtime_error("[ERROR] Only one multipath surface per simulation is supported");
	}
	_multipath_surface = surface;
}

void World::processMultipath()
{
	if (_multipath_surface)
	{
		for (const auto plat : _platforms)
		{
			_platforms.push_back(createMultipathDual(plat, _multipath_surface));
		}
		for (const auto recv : _receivers)
		{
			_receivers.push_back(createMultipathDual(recv, _multipath_surface));
		}
		for (const auto trans : _transmitters)
		{
			_transmitters.push_back(createMultipathDual(trans, _multipath_surface));
		}
		delete _multipath_surface;
		_multipath_surface = nullptr;
	}
}
