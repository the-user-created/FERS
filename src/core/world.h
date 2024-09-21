// world.h
// Simulator World Object
// The World contains all the other objects in the simulator's worldview
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 25 April 2006

#ifndef WORLD_H
#define WORLD_H

#include <memory>
#include <unordered_map>
#include <vector>

#include "math_utils/multipath_surface.h"
#include "radar/radar_system.h"
#include "radar/target.h"
#include "signal_processing/radar_signal.h"
#include "timing/prototype_timing.h"

namespace core
{
	class World
	{
	public:
		World() = default;

		~World() = default;

		void add(std::unique_ptr<radar::Platform> plat);

		void add(std::unique_ptr<radar::Transmitter> trans);

		void add(std::unique_ptr<radar::Receiver> recv);

		void add(std::unique_ptr<radar::Target> target);

		void add(std::unique_ptr<signal::RadarSignal> pulse);

		void add(std::unique_ptr<antenna::Antenna> antenna);

		void add(std::unique_ptr<timing::PrototypeTiming> timing);

		void addMultipathSurface(std::unique_ptr<math::MultipathSurface> surface);

		[[nodiscard]] signal::RadarSignal* findSignal(const std::string& name);

		[[nodiscard]] antenna::Antenna* findAntenna(const std::string& name);

		[[nodiscard]] timing::PrototypeTiming* findTiming(const std::string& name);

		[[nodiscard]] const std::vector<std::unique_ptr<radar::Target>>& getTargets() const noexcept
		{
			return _targets;
		}

		[[nodiscard]] const std::vector<std::unique_ptr<radar::Receiver>>& getReceivers() const noexcept
		{
			return _receivers;
		}

		[[nodiscard]] const std::vector<std::unique_ptr<radar::Transmitter>>& getTransmitters() const noexcept
		{
			return _transmitters;
		}

		void processMultipath();

	private:
		std::vector<std::unique_ptr<radar::Platform>> _platforms;
		std::vector<std::unique_ptr<radar::Transmitter>> _transmitters;
		std::vector<std::unique_ptr<radar::Receiver>> _receivers;
		std::vector<std::unique_ptr<radar::Target>> _targets;
		std::unordered_map<std::string, std::unique_ptr<signal::RadarSignal>> _pulses;
		std::unordered_map<std::string, std::unique_ptr<antenna::Antenna>> _antennas;
		std::unordered_map<std::string, std::unique_ptr<timing::PrototypeTiming>> _timings;
		std::unique_ptr<math::MultipathSurface> _multipath_surface;
	};
}

#endif
