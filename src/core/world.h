// world.h
// Simulator World Object
// The World contains all the other objects in the simulator's worldview
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 25 April 2006

#ifndef WORLD_H
#define WORLD_H

#include <map>
#include <string>
#include <vector>

namespace rs
{
	class Receiver;
	class Transmitter;
	class RadarSignal;
	class Antenna;
	class Target;
	class Platform;
	class PrototypeTiming;
	class MultipathSurface;

	class World
	{
	public:
		World() : _multipath_surface(nullptr)
		{
		}

		~World();

		void add(Platform* plat)
		{
			_platforms.push_back(plat);
		}

		void add(Transmitter* trans)
		{
			_transmitters.push_back(trans);
		}

		void add(Receiver* recv)
		{
			_receivers.push_back(recv);
		}

		void add(Target* target)
		{
			_targets.push_back(target);
		}

		void add(RadarSignal* pulse);

		void add(Antenna* antenna);

		void add(PrototypeTiming* timing);

		void addMultipathSurface(MultipathSurface* surface);

		RadarSignal* findSignal(const std::string& name)
		{
			return _pulses[name];
		}

		Antenna* findAntenna(const std::string& name)
		{
			return _antennas[name];
		}

		PrototypeTiming* findTiming(const std::string& name)
		{
			return _timings[name];
		}

		[[nodiscard]] std::vector<Target*> getTargets() const
		{
			return _targets;
		}

		[[nodiscard]] std::vector<Receiver*> getReceivers() const
		{
			return _receivers;
		}

		[[nodiscard]] std::vector<Transmitter*> getTransmitters() const
		{
			return _transmitters;
		}

		void processMultipath();

	private:
		std::vector<Platform*> _platforms;
		std::vector<Transmitter*> _transmitters;
		std::vector<Receiver*> _receivers;
		std::vector<Target*> _targets;
		std::map<std::string, RadarSignal*> _pulses;
		std::map<std::string, Antenna*> _antennas;
		std::map<std::string, PrototypeTiming*> _timings;
		MultipathSurface* _multipath_surface;
	};
}

#endif
