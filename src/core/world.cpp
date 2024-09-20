// world.cpp
// Implementation of simulator world object
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 25 April 2006

#include "world.h"

#include <algorithm>

#include "math_utils/multipath_surface.h"
#include "radar/radar_system.h"
#include "radar/target.h"
#include "timing/prototype_timing.h"

using namespace rs;

void World::add(std::unique_ptr<RadarSignal> pulse)
{
	if (findSignal(pulse->getName()))
	{
		throw std::runtime_error("A pulse with the name " + pulse->getName() + " already exists.");
	}
	_pulses[pulse->getName()] = std::move(pulse);
}

void World::add(std::unique_ptr<Antenna> antenna)
{
	if (findAntenna(antenna->getName()))
	{
		throw std::runtime_error("An antenna with the name " + antenna->getName() + " already exists.");
	}
	_antennas[antenna->getName()] = std::move(antenna);
}

void World::add(std::unique_ptr<PrototypeTiming> timing)
{
	if (findTiming(timing->getName()))
	{
		throw std::runtime_error("A timing source with the name " + timing->getName() + " already exists.");
	}
	_timings[timing->getName()] = std::move(timing);
}

void World::addMultipathSurface(std::unique_ptr<MultipathSurface> surface)
{
	if (_multipath_surface)
	{
		throw std::runtime_error("Only one multipath surface per simulation is supported");
	}
	_multipath_surface = std::move(surface);
}

void World::processMultipath()
{
	if (_multipath_surface)
	{
		// TODO: See if there is a way to do this without copying across the vector
		std::vector<std::unique_ptr<Platform>> new_duals;
		for (const auto& plat : _platforms)
		{
		    new_duals.push_back(std::unique_ptr<Platform>(createMultipathDual(plat.get(), _multipath_surface.get())));
		}
		_platforms.insert(_platforms.end(), std::make_move_iterator(new_duals.begin()), std::make_move_iterator(new_duals.end()));

		for (const auto& recv : _receivers)
		{
			_receivers.push_back(std::vector<std::unique_ptr<Receiver>>::value_type(createMultipathDual(recv.get(), _multipath_surface.get())));
		}

		for (const auto& trans : _transmitters)
		{
			_transmitters.push_back(std::vector<std::unique_ptr<Transmitter>>::value_type(createMultipathDual(trans.get(), _multipath_surface.get())));
		}

		_multipath_surface.reset();
	}
}
