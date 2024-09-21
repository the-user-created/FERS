// world.cpp
// Implementation of simulator world object
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 25 April 2006

#include "world.h"

#include "antenna/antenna_factory.h"
#include "radar/radar_system.h"
#include "signal_processing/radar_signal.h"
#include "timing/prototype_timing.h"

using signal::RadarSignal;
using antenna::Antenna;
using timing::PrototypeTiming;
using math::MultipathSurface;
using radar::Platform;
using radar::Receiver;
using radar::Transmitter;
using radar::Target;

namespace core
{
	void World::add(std::unique_ptr<Platform> plat) { _platforms.push_back(std::move(plat)); }

	void World::add(std::unique_ptr<Transmitter> trans) { _transmitters.push_back(std::move(trans)); }

	void World::add(std::unique_ptr<Receiver> recv) { _receivers.push_back(std::move(recv)); }

	void World::add(std::unique_ptr<Target> target) { _targets.push_back(std::move(target)); }

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

	RadarSignal* World::findSignal(const std::string& name) { return _pulses[name].get(); }

	Antenna* World::findAntenna(const std::string& name) { return _antennas[name].get(); }

	PrototypeTiming* World::findTiming(const std::string& name) { return _timings[name].get(); }

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
}
