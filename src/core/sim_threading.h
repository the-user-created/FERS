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
#include <functional>
#include <utility>

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
	 * @brief Class for managing tasks in a multithreaded environment.
	 *
	 * This class provides a simple interface for managing tasks in a multithreaded environment.
	 * It defines a task as a std::function object and executes the task when called.
	 */
	class TaskThread
	{
	public:
		/**
		 * @brief Alias for a task function.
		 *
		 * Defines a task function as a std::function object.
		 */
		using Task = std::function<void()>;

		/**
		 * @brief Constructs a TaskThread object.
		 *
		 * Constructs a TaskThread object with the specified task and task name.
		 *
		 * @param task The task to be executed.
		 * @param taskName The name of the task.
		 */
		TaskThread(Task task, std::string taskName) : _task(std::move(task)), _task_name(std::move(taskName)) {}

		/**
		 * @brief Executes the task.
		 *
		 * Executes the task stored in the object.
		 */
		void operator()() const;

	private:
		Task _task; ///< The task to be executed.
		std::string _task_name; ///< The name of the task.
	};

	/**
	 * @brief Runs the simulation in a multithreaded environment.
	 *
	 * This function runs the simulation in a multithreaded environment, using the specified number of threads.
	 *
	 * @param threadLimit The maximum number of threads to use.
	 * @param world Pointer to the world object that holds the simulation environment.
	 */
	void runThreadedSim(unsigned threadLimit, const World* world);
}
