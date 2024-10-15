/**
 * @file sim_threading.h
 * @brief Header file for threaded simulation management.
 *
 * This file contains the definitions and classes necessary for running a threaded simulation.
 * It includes classes that handle simulations for transmitter-receiver pairs and rendering processes for each receiver.
 *
 * @authors David Young, Marc Brooker
 * @date 2006-07-19
 */

#pragma once

#include <exception>
#include <functional>
#include <utility>

#include "config.h"

namespace pool {
	class ThreadPool;
}

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
	 */
	class RangeError final : public std::exception
	{
	public:
		/**
		 * @brief Provides the error message for the exception.
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
		 */
		using Task = std::function<void()>;

		/**
		 * @brief Constructs a TaskThread object.
		 *
		 * @param task The task to be executed.
		 * @param taskName The name of the task.
		 */
		TaskThread(Task task, std::string taskName) : _task(std::move(task)), _task_name(std::move(taskName)) {}

		/**
		 * @brief Executes the task.
		 */
		void operator()() const;

	private:
		Task _task; ///< The task to be executed.
		std::string _task_name; ///< The name of the task.
	};

	void runThreadedSim(const World* world, pool::ThreadPool& pool);
}
