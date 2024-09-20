// world.h
// Simulator World Object
// The World contains all the other objects in the simulator's worldview
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 25 April 2006

#ifndef WORLD_H
#define WORLD_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "math_utils/multipath_surface.h"
#include "radar/target.h"
#include "timing/prototype_timing.h"

namespace rs
{
	class Receiver;
	class Transmitter;
	class RadarSignal;
	class Antenna;
	class Platform;

	class World
	{
	public:
		World() = default;

		~World() = default;

		void add(std::unique_ptr<Platform> plat) { _platforms.push_back(std::move(plat)); }

		void add(std::unique_ptr<Transmitter> trans) { _transmitters.push_back(std::move(trans)); }

		void add(std::unique_ptr<Receiver> recv) { _receivers.push_back(std::move(recv)); }

		void add(std::unique_ptr<Target> target) { _targets.push_back(std::move(target)); }

		void add(std::unique_ptr<RadarSignal> pulse);

		void add(std::unique_ptr<Antenna> antenna);

		void add(std::unique_ptr<PrototypeTiming> timing);

		void addMultipathSurface(std::unique_ptr<MultipathSurface> surface);

		RadarSignal* findSignal(const std::string& name) { return _pulses[name].get(); }

		Antenna* findAntenna(const std::string& name) { return _antennas[name].get(); }

		PrototypeTiming* findTiming(const std::string& name) { return _timings[name].get(); }

		[[nodiscard]] const std::vector<std::unique_ptr<Target>>& getTargets() const { return _targets; }

		[[nodiscard]] const std::vector<std::unique_ptr<Receiver>>& getReceivers() const { return _receivers; }

		[[nodiscard]] const std::vector<std::unique_ptr<Transmitter>>& getTransmitters() const { return _transmitters; }

		void processMultipath();

	private:
		std::vector<std::unique_ptr<Platform>> _platforms;
		std::vector<std::unique_ptr<Transmitter>> _transmitters;
		std::vector<std::unique_ptr<Receiver>> _receivers;
		std::vector<std::unique_ptr<Target>> _targets;
		std::map<std::string, std::unique_ptr<RadarSignal>> _pulses;
		std::map<std::string, std::unique_ptr<Antenna>> _antennas;
		std::map<std::string, std::unique_ptr<PrototypeTiming>> _timings;
		std::unique_ptr<MultipathSurface> _multipath_surface;
	};
}

#endif
