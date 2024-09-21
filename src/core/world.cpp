// world.cpp
// Implementation of simulator world object
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 25 April 2006

#include "world.h"

#include <ranges>

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
	using namespace std::string_literals;

	void World::add(std::unique_ptr<Platform> plat) { _platforms.push_back(std::move(plat)); }

	void World::add(std::unique_ptr<Transmitter> trans) { _transmitters.push_back(std::move(trans)); }

	void World::add(std::unique_ptr<Receiver> recv) { _receivers.push_back(std::move(recv)); }

	void World::add(std::unique_ptr<Target> target) { _targets.push_back(std::move(target)); }

	void World::add(std::unique_ptr<RadarSignal> pulse)
	{
		if (_pulses.contains(pulse->getName()))
		{
			throw std::runtime_error("A pulse with the name " + pulse->getName() + " already exists.");
		}
		_pulses[pulse->getName()] = std::move(pulse);
	}

	void World::add(std::unique_ptr<Antenna> antenna)
	{
		if (_antennas.contains(antenna->getName()))
		{
			throw std::runtime_error("An antenna with the name " + antenna->getName() + " already exists.");
		}
		_antennas[antenna->getName()] = std::move(antenna);
	}

	void World::add(std::unique_ptr<PrototypeTiming> timing)
	{
		if (_timings.contains(timing->getName()))
		{
			throw std::runtime_error("A timing source with the name " + timing->getName() + " already exists.");
		}
		_timings[timing->getName()] = std::move(timing);
	}

	void World::addMultipathSurface(std::unique_ptr<MultipathSurface> surface)
	{
		if (_multipath_surface) { throw std::runtime_error("Only one multipath surface per simulation is supported."); }
		_multipath_surface = std::move(surface);
	}

	RadarSignal* World::findSignal(const std::string& name)
	{
		return _pulses.contains(name) ? _pulses[name].get() : nullptr;
	}

	Antenna* World::findAntenna(const std::string& name)
	{
		return _antennas.contains(name) ? _antennas[name].get() : nullptr;
	}

	PrototypeTiming* World::findTiming(const std::string& name)
	{
		return _timings.contains(name) ? _timings[name].get() : nullptr;
	}

	void World::processMultipath()
	{
		if (_multipath_surface)
		{
			const auto append_multipath_duals = [this](auto& collection)
			{
				const size_t initial_size = collection.size();
				collection.reserve(initial_size * 2);
				for (size_t i = 0; i < initial_size; ++i)
				{
					collection.push_back(std::unique_ptr<typename std::decay_t<decltype(collection[i])>::element_type>(
						createMultipathDual(collection[i].get(), _multipath_surface.get())));
				}
			};

			append_multipath_duals(_platforms);
			append_multipath_duals(_receivers);
			append_multipath_duals(_transmitters);

			_multipath_surface.reset();
		}
	}
}
