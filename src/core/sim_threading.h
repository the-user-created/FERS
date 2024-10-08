/**
 * @file sim_threading.h
 * @brief Header file for threaded simulation management.
 *
 * This file contains the definitions and classes necessary for running a threaded simulation.
 * It includes classes that handle simulations for transmitter-receiver pairs and rendering processes for each receiver.
 * The goal is to use multithreading to run simulations in parallel,
 * improving the performance and efficiency of the system.
 *
 * @authors David Young, Marc Brooker
 * @date 2006-07-19
 */

#pragma once

#include <exception>

#include "config.h"

namespace radar
{
	class Receiver;
	class Transmitter;
}

namespace core
{
	class World;

	/**
     * @struct ReResults
     * @brief Stores the results of a radar simulation.
     *
     * Contains information about power, delay, Doppler shift, phase,
     * and noise temperature resulting from radar simulations.
     */
	struct ReResults
	{
		RealType power; /**< Power of the radar signal. */
		RealType delay; /**< Signal delay in time. */
		RealType doppler; /**< Doppler shift of the radar signal. */
		RealType phase; /**< Phase of the radar signal. */
		RealType noise_temperature; /**< Noise temperature affecting the radar signal. */
	};

	/**
	 * @class RangeError
	 * @brief Exception class for range calculation errors.
	 *
	 * Thrown when an error occurs during the range calculations in the radar simulation.
	 * This error class provides a custom exception message for issues encountered while processing range estimation.
	 */
	class RangeError final : public std::exception
	{
	public:
		/**
		 * @brief Provides the error message for the exception.
		 *
		 * Returns a constant character pointer containing the error message.
		 *
		 * @return The error message string.
		 */
		[[nodiscard]] const char* what() const noexcept override { return "Range error in RE calculations"; }
	};

	/**
	 * @class SimThread
	 * @brief Manages the simulation of a transmitter-receiver pair.
	 *
	 * This class handles the simulation of a single transmitter and receiver pair in a multithreaded environment.
	 * It executes the simulation in a separate thread, allowing multiple simulations to run in parallel.
	 */
	class SimThread
	{
	public:
		/**
		 * @brief Constructs a SimThread instance.
		 *
		 * Initializes the SimThread with a pointer to a transmitter,
		 * a receiver, and a world object that holds the simulation environment.
		 *
		 * @param transmitter Pointer to the radar transmitter.
		 * @param receiver Pointer to the radar receiver.
		 * @param world Pointer to the simulation environment.
		 */
		SimThread(const radar::Transmitter* transmitter, radar::Receiver* receiver, const World* world)
			: _trans(transmitter), _recv(receiver), _world(world) {}

		/**
		 * @brief Runs the simulation thread.
		 *
		 * This function is executed when the thread starts,
		 * running the simulation for the associated transmitter and receiver.
		 */
		void operator()() const;

	private:
		const radar::Transmitter* _trans; /**< Pointer to the associated transmitter. */
		radar::Receiver* _recv; /**< Pointer to the associated receiver. */
		const World* _world; /**< Pointer to the world object representing the simulation environment. */
	};

	/**
	 * @class RenderThread
	 * @brief Handles the rendering process for a radar receiver.
	 *
	 * This class manages the rendering of simulation data for a radar receiver.
	 * It runs in a separate thread to ensure the rendering process does not block other operations.
	 */
	class RenderThread
	{
	public:
		/**
		 * @brief Constructs a RenderThread instance.
		 *
		 * Initializes the RenderThread with a pointer to a radar receiver.
		 *
		 * @param recv Pointer to the radar receiver.
		 */
		explicit RenderThread(radar::Receiver* recv) : _recv(recv) {}

		/**
		 * @brief Runs the rendering thread.
		 *
		 * This function is executed when the thread starts, handling the rendering process for the receiver.
		 */
		void operator()() const;

	private:
		radar::Receiver* _recv; /**< Pointer to the receiver for which rendering is handled. */
	};

	/**
	 * @brief Runs the simulation in a multithreaded environment.
	 *
	 * Manages the threading process for running simulations in parallel, up to a specified thread limit.
	 * It handles the initialization of threads and ensures that the correct number of simulations is run simultaneously.
	 *
	 * @param threadLimit The maximum number of threads to use.
	 * @param world Pointer to the world object that holds the simulation environment.
	 */
	void runThreadedSim(unsigned threadLimit, const World* world);
}
