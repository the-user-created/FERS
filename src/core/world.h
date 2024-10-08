/**
* @file world.h
* @brief Header file for the World class in the simulator.
*
* The World-class encapsulates the simulation world,
* managing various radar-related objects such as platforms, transmitters,
* receivers, targets, antennas, radar signals, and timing sources.
* It provides functionality for adding and retrieving these objects,
* as well as processing multipath effects within the simulation.
*
* @authors David Young, Marc Brooker
* @date 2006-04-25
*/

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "antenna/antenna_factory.h"
#include "math/multipath_surface.h"
#include "radar/platform.h"
#include "radar/receiver.h"
#include "radar/target.h"
#include "radar/transmitter.h"
#include "signal/radar_signal.h"
#include "timing/prototype_timing.h"

namespace core
{
	/**
	* @class World
	* @brief The World class manages the simulator environment.
	*
	* The World class is responsible for maintaining and organizing various radar simulation objects,
	* including platforms, transmitters, receivers, targets, antennas, radar signals, and timing mechanisms.
	* It provides methods for adding and retrieving these components
	* and handles multipath surface effects during simulation.
	*/
	class World
	{
	public:
		/**
	    * @brief Default constructor.
	    *
	    * Constructs an empty simulation world.
	    */
		World() = default;

		/**
		* @brief Default destructor.
		*
		* Cleans up resources when the world object is destroyed.
		*/
		~World() noexcept = default;

		/**
		* @brief Adds a radar platform to the simulation world.
		*
		* @param plat A unique pointer to a Platform object.
		*/
		void add(std::unique_ptr<radar::Platform> plat) noexcept;

		/**
		* @brief Adds a radar transmitter to the simulation world.
		*
		* @param trans A unique pointer to a Transmitter object.
		*/
		void add(std::unique_ptr<radar::Transmitter> trans) noexcept;

		/**
		* @brief Adds a radar receiver to the simulation world.
		*
		* @param recv A unique pointer to a Receiver object.
		*/
		void add(std::unique_ptr<radar::Receiver> recv) noexcept;

		/**
		* @brief Adds a radar target to the simulation world.
		*
		* @param target A unique pointer to a Target object.
		*/
		void add(std::unique_ptr<radar::Target> target) noexcept;

		/**
		* @brief Adds a radar signal (pulse) to the simulation world.
		*
		* @param pulse A unique pointer to a RadarSignal object.
		* @throws std::runtime_error if a pulse with the same name already exists.
		*/
		void add(std::unique_ptr<signal::RadarSignal> pulse);

		/**
		* @brief Adds an antenna to the simulation world.
		*
		* @param antenna A unique pointer to an Antenna object.
		* @throws std::runtime_error if an antenna with the same name already exists.
		*/
		void add(std::unique_ptr<antenna::Antenna> antenna);

		/**
		* @brief Adds a timing source to the simulation world.
		*
		* @param timing A unique pointer to a PrototypeTiming object.
		* @throws std::runtime_error if a timing source with the same name already exists.
		*/
		void add(std::unique_ptr<timing::PrototypeTiming> timing);

		/**
		* @brief Adds a multipath surface to the simulation world.
		*
		* @param surface A unique pointer to a MultipathSurface object.
		* @throws std::runtime_error if a multipath surface already exists in the simulation.
		*/
		void addMultipathSurface(std::unique_ptr<math::MultipathSurface> surface);

		/**
		* @brief Finds a radar signal by name.
		*
		* @param name The name of the radar signal to find.
		* @return A pointer to the RadarSignal if found, or nullptr if not found.
		*/
		[[nodiscard]] signal::RadarSignal* findSignal(const std::string& name);

		/**
		* @brief Finds an antenna by name.
		*
		* @param name The name of the antenna to find.
		* @return A pointer to the Antenna if found, or nullptr if not found.
		*/
		[[nodiscard]] antenna::Antenna* findAntenna(const std::string& name);

		/**
		* @brief Finds a timing source by name.
		*
		* @param name The name of the timing source to find.
		* @return A pointer to the PrototypeTiming if found, or nullptr if not found.
		*/
		[[nodiscard]] timing::PrototypeTiming* findTiming(const std::string& name);

		/**
		* @brief Retrieves the list of radar targets.
		*
		* @return A const reference to a vector of unique pointers to Target objects.
		*/
		[[nodiscard]] const std::vector<std::unique_ptr<radar::Target>>& getTargets() const noexcept
		{
			return _targets;
		}

		/**
		* @brief Retrieves the list of radar receivers.
		*
		* @return A const reference to a vector of unique pointers to Receiver objects.
		*/
		[[nodiscard]] const std::vector<std::unique_ptr<radar::Receiver>>& getReceivers() const noexcept
		{
			return _receivers;
		}

		/**
		* @brief Retrieves the list of radar transmitters.
		*
		* @return A const reference to a vector of unique pointers to Transmitter objects.
		*/
		[[nodiscard]] const std::vector<std::unique_ptr<radar::Transmitter>>& getTransmitters() const noexcept
		{
			return _transmitters;
		}

		/**
		* @brief Processes multipath surface interactions.
		*
		* If a multipath surface is present, this function will modify the simulation objects to account for multipath effects.
		*/
		void processMultipath();

	private:
		/**
		* @brief Stores the platforms in the simulation world.
		*/
		std::vector<std::unique_ptr<radar::Platform>> _platforms;

		/**
		* @brief Stores the transmitters in the simulation world.
		*/
		std::vector<std::unique_ptr<radar::Transmitter>> _transmitters;

		/**
		* @brief Stores the receivers in the simulation world.
		*/
		std::vector<std::unique_ptr<radar::Receiver>> _receivers;

		/**
		* @brief Stores the targets in the simulation world.
		*/
		std::vector<std::unique_ptr<radar::Target>> _targets;

		/**
		* @brief Maps signal names to RadarSignal objects.
		*/
		std::unordered_map<std::string, std::unique_ptr<signal::RadarSignal>> _pulses;

		/**
		* @brief Maps antenna names to Antenna objects.
		*/
		std::unordered_map<std::string, std::unique_ptr<antenna::Antenna>> _antennas;

		/**
		* @brief Maps timing source names to PrototypeTiming objects.
		*/
		std::unordered_map<std::string, std::unique_ptr<timing::PrototypeTiming>> _timings;

		/**
		* @brief Stores the multipath surface in the simulation world.
		*/
		std::unique_ptr<math::MultipathSurface> _multipath_surface;
	};
}
