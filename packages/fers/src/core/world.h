/**
* @file world.h
* @brief Header file for the World class in the simulator.
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
	*/
	class World
	{
	public:
		World() = default;

		~World() noexcept = default;

		World(const World&) = delete;

		World& operator=(const World&) = delete;

		World(World&&) = delete;

		World& operator=(World&&) = delete;

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
		void add(std::unique_ptr<fers_signal::RadarSignal> pulse);

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
		* @brief Finds a radar signal by name.
		*
		* @param name The name of the radar signal to find.
		* @return A pointer to the RadarSignal if found, or nullptr if not found.
		*/
		[[nodiscard]] fers_signal::RadarSignal* findSignal(const std::string& name);

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

	private:
		std::vector<std::unique_ptr<radar::Platform>> _platforms;

		std::vector<std::unique_ptr<radar::Transmitter>> _transmitters;

		std::vector<std::unique_ptr<radar::Receiver>> _receivers;

		std::vector<std::unique_ptr<radar::Target>> _targets;

		std::unordered_map<std::string, std::unique_ptr<fers_signal::RadarSignal>> _pulses;

		std::unordered_map<std::string, std::unique_ptr<antenna::Antenna>> _antennas;

		std::unordered_map<std::string, std::unique_ptr<timing::PrototypeTiming>> _timings;
	};
}
