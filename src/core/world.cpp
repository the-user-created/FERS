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
	if (_multipath_surface) { throw std::runtime_error("Only one multipath surface per simulation is supported"); }
	_multipath_surface = std::move(surface);
}

void World::processMultipath()
{
	if (_multipath_surface)
	{
		// Store the original sizes of the vectors before modification
		const size_t platform_count = _platforms.size();
		const size_t receiver_count = _receivers.size();
		const size_t transmitter_count = _transmitters.size();

		// Reserve space to avoid multiple reallocations
		_platforms.reserve(platform_count * 2);
		_receivers.reserve(receiver_count * 2);
		_transmitters.reserve(transmitter_count * 2);

		// Create and append new duals for platforms
		for (size_t i = 0; i < platform_count; ++i)
		{
			_platforms.emplace_back(createMultipathDual(_platforms[i].get(), _multipath_surface.get()));
		}

		// Create and append new duals for receivers
		for (size_t i = 0; i < receiver_count; ++i)
		{
			_receivers.emplace_back(createMultipathDual(_receivers[i].get(), _multipath_surface.get()));
		}

		// Create and append new duals for transmitters
		for (size_t i = 0; i < transmitter_count; ++i)
		{
			_transmitters.emplace_back(createMultipathDual(_transmitters[i].get(), _multipath_surface.get()));
		}

		_multipath_surface.reset();
	}
}
