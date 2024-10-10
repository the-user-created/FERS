/**
* @file sim_threading.cpp
* @brief Thread management for the simulator.
*
* This file contains the implementation of the classes and functions necessary for running a threaded simulation.
* It includes classes that handle simulations for transmitter-receiver pairs and rendering processes for each receiver.
* The goal is to use multithreading to run simulations in parallel,
* improving the performance and efficiency of the system.
*
* @authors David Young, Marc Brooker
* @date 2006-07-19
*/

#include "sim_threading.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>
#include <bits/chrono.h>

#include "parameters.h"
#include "thread_pool.h"
#include "world.h"
#include "core/logging.h"
#include "interpolation/interpolation_point.h"
#include "math/geometry_ops.h"
#include "radar/radar_obj.h"
#include "radar/target.h"
#include "serial/response.h"
#include "signal/radar_signal.h"

using radar::Transmitter;
using radar::Receiver;
using radar::Target;
using math::SVec3;
using signal::RadarSignal;
using radar::TransmitterPulse;
using serial::Response;
using logging::Level;

// Counter of currently running threads
std::atomic threads{0};

// Flag to set if a thread encounters an error
std::atomic error{false};

namespace
{
	// =================================================================================================================
	//
	// THREAD CLASSES HELPERS
	//
	// =================================================================================================================

	/**
	 * @brief Performs radar simulation for a given transmitter, receiver, and target.
	 *
	 * This function calculates radar simulation results, including power, delay, Doppler shift, phase,
	 * and noise temperature based on the position and characteristics of the transmitter, receiver, and target.
	 * It also accounts for multipath effects, propagation loss, and Doppler shift in the final calculations.
	 *
	 * @param trans Pointer to the radar transmitter.
	 * @param recv Pointer to the radar receiver.
	 * @param targ Pointer to the radar target.
	 * @param time The time at which the simulation is being run.
	 * @param length The duration of the radar pulse.
	 * @param wave Pointer to the radar signal object.
	 * @param results Struct to store the results of the simulation.
	 * @throws core::RangeError If the transmitter or receiver is too close to the target for an accurate simulation.
	 */
	void solveRe(const Transmitter* trans, const Receiver* recv, const Target* targ,
	             const std::chrono::duration<RealType>& time, const std::chrono::duration<RealType>& length,
	             const RadarSignal* wave, core::ReResults& results)
	{
		// Get initial positions and distance vectors
		const auto transmitter_position = trans->getPosition(time.count());
		const auto receiver_position = recv->getPosition(time.count());
		const auto target_position = targ->getPosition(time.count());

		SVec3 transmitter_to_target_vector{target_position - transmitter_position};
		SVec3 receiver_to_target_vector{target_position - receiver_position};

		const auto transmitter_to_target_distance = transmitter_to_target_vector.length;
		const auto receiver_to_target_distance = receiver_to_target_vector.length;

		if (transmitter_to_target_distance <= EPSILON || receiver_to_target_distance <= EPSILON)
		{
			LOG(Level::FATAL, "Transmitter or Receiver too close to Target for accurate simulation");
			throw core::RangeError();
		}

		// Normalize distance vectors
		transmitter_to_target_vector.length = 1;
		receiver_to_target_vector.length = 1;

		results.delay = (transmitter_to_target_distance + receiver_to_target_distance) / params::c();

		const auto rcs = targ->getRcs(transmitter_to_target_vector, receiver_to_target_vector);
		const auto wavelength = params::c() / wave->getCarrier();

		const auto transmitter_gain = trans->
			getGain(transmitter_to_target_vector, trans->getRotation(time.count()), wavelength);
		const auto receiver_gain = recv->getGain(receiver_to_target_vector,
		                                         recv->getRotation(results.delay + time.count()),
		                                         wavelength);

		results.power = transmitter_gain * receiver_gain * rcs / (4 * PI);

		// Propagation loss calculation
		if (!recv->checkFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS))
		{
			const RealType distance_product = transmitter_to_target_distance * receiver_to_target_distance;
			results.power *= std::pow(wavelength, 2) / (std::pow(4 * PI, 2) * std::pow(
				distance_product, 2));
		}

		// Apply multipath factors if applicable
		if (trans->getMultipathDual()) { results.power *= trans->getMultipathFactor(); }
		if (recv->getMultipathDual()) { results.power *= recv->getMultipathFactor(); }

		results.phase = -results.delay * 2 * PI * wave->getCarrier();

		// End position and Doppler calculation
		const auto transvec_end = SVec3(
			targ->getPosition(time.count() + length.count()) - trans->getPosition(time.count() + length.count()));
		const auto recvvec_end = SVec3(
			targ->getPosition(time.count() + length.count()) - recv->getPosition(time.count() + length.count()));

		const RealType rt_end = transvec_end.length;
		const RealType rr_end = recvvec_end.length;

		if (rt_end <= EPSILON || rr_end <= EPSILON)
		{
			LOG(Level::FATAL, "Transmitter or Receiver too close to Target for accurate simulation");
			throw core::RangeError();
		}

		const RealType v_r = (rr_end - receiver_to_target_distance) / length.count();
		const RealType v_t = (rt_end - transmitter_to_target_distance) / length.count();

		const auto c = params::c();
		results.doppler = std::sqrt((1 + v_r / c) / (1 - v_r / c)) * std::sqrt((1 + v_t / c) / (1 - v_t / c));

		// Set noise temperature
		results.noise_temperature = recv->getNoiseTemperature(recv->getRotation(time.count() + results.delay));
	}

	/**
	 * @brief Performs direct radar simulation between a transmitter and receiver without a target.
	 *
	 * This function calculates the simulation results when the radar signal travels directly
	 * between the transmitter and receiver, without any interaction with a target.
	 * It computes delay, power, Doppler shift,
	 * and phase based on the relative positions of the transmitter and receiver.
	 *
	 * @param trans Pointer to the radar transmitter.
	 * @param recv Pointer to the radar receiver.
	 * @param time The time at which the simulation is being run.
	 * @param length The duration of the radar pulse.
	 * @param wave Pointer to the radar signal object.
	 * @param results Struct to store the results of the simulation.
	 * @throws core::RangeError If the transmitter or receiver is too close for accurate simulation.
	 */
	void solveReDirect(const Transmitter* trans, const Receiver* recv, const std::chrono::duration<RealType>& time,
	                   const std::chrono::duration<RealType>& length, const RadarSignal* wave, core::ReResults& results)
	{
		// Get positions
		const auto tpos = trans->getPosition(time.count());
		const auto rpos = recv->getPosition(time.count());

		// Calculate distance between transmitter and receiver
		const SVec3 transvec{tpos - rpos};
		const RealType distance = transvec.length;

		if (distance <= EPSILON)
		{
			LOG(Level::FATAL, "Transmitter or Receiver too close to Target for accurate simulation");
			throw core::RangeError();
		}

		// Time delay calculation
		results.delay = distance / params::c();

		// Calculate gains and wavelength
		const RealType wavelength = params::c() / wave->getCarrier();
		const RealType transmitter_gain = trans->getGain(transvec, trans->getRotation(time.count()), wavelength);
		const RealType receiver_gain = recv->getGain(SVec3(rpos - tpos),
		                                             recv->getRotation(time.count() + results.delay),
		                                             wavelength);

		// Power calculation
		results.power = transmitter_gain * receiver_gain * wavelength * wavelength / (4 * PI);

		// Propagation loss
		if (!recv->checkFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS)) { results.power /= 4 * PI * distance * distance; }

		// Doppler shift calculation
		const auto trpos_end = SVec3(
			trans->getPosition(time.count() + length.count()) - recv->getPosition(time.count() + length.count()));
		const RealType r_end = trpos_end.length;
		const RealType doppler_shift = (r_end - distance) / length.count();

		results.doppler = (params::c() + doppler_shift) / (params::c() - doppler_shift);

		// Multipath conditions: if either transmitter or receiver has multipath, zero the power
		if (trans->getMultipathDual() || recv->getMultipathDual()) { results.power = 0; }

		// Phase calculation, ensuring the phase is wrapped within [0, 2π]
		results.phase = std::fmod(results.delay * 2 * PI * wave->getCarrier(), 2 * PI);

		// Set noise temperature
		results.noise_temperature = recv->getNoiseTemperature(recv->getRotation(time.count() + results.delay));
	}

	/**
	 * @brief Adds a direct response for a transmitter-receiver pair.
	 *
	 * This function simulates the direct interaction between a transmitter and receiver,
	 * adding the resulting radar response data to the receiver.
	 * It runs the simulation for each sample point in the radar signal.
	 *
	 * @param trans Pointer to the radar transmitter.
	 * @param recv Pointer to the radar receiver.
	 * @param signal Pointer to the radar signal pulse being simulated.
	 * @throws core::RangeError If the transmitter or receiver is too close for accurate simulation.
	 */
	void addDirect(const Transmitter* trans, Receiver* recv, const TransmitterPulse* signal)
	{
		// Exit early if the transmitter is attached to the receiver
		if (trans->getAttached() == recv) { return; }

		// Convert time variables to std::chrono for better type safety
		const auto start_time = std::chrono::duration<RealType>(signal->time);
		const auto end_time = start_time + std::chrono::duration<RealType>(signal->wave->getLength());
		const auto sample_time = std::chrono::duration<RealType>(1.0 / params::cwSampleRate());
		const int point_count = static_cast<int>(std::ceil(signal->wave->getLength() / sample_time.count()));

		// Create a response object for this signal
		auto response = std::make_unique<Response>(signal->wave, trans);

		try
		{
			// Iterate through sample points
			for (int i = 0; i <= point_count; ++i)
			{
				// Calculate current time
				const auto current_time = i < point_count ? start_time + i * sample_time : end_time;

				// Compute simulation results
				core::ReResults results{};
				solveReDirect(trans, recv, current_time, sample_time, signal->wave, results);

				// Add interpolation point with designated initializers
				interp::InterpPoint point{
					.power = results.power,
					.time = current_time.count() + results.delay,
					.delay = results.delay,
					.doppler = results.doppler,
					.phase = results.phase,
					.noise_temperature = results.noise_temperature
				};
				response->addInterpPoint(point);
			}
		}
		catch (const core::RangeError&)
		{
			LOG(Level::FATAL, "Receiver or Transmitter too close to Target for accurate simulation");
			throw core::RangeError();
		}

		// Add response to receiver
		recv->addResponse(std::move(response));
	}

	/**
	 * @brief Simulates the radar signal interaction with a target.
	 *
	 * This function runs a radar simulation for a specific target,
	 * calculating the results for each time point of the radar pulse.
	 * The results are added to the receiver as a radar response.
	 *
	 * @param trans Pointer to the radar transmitter.
	 * @param recv Pointer to the radar receiver.
	 * @param targ Pointer to the radar target.
	 * @param signal Pointer to the radar signal pulse being simulated.
	 * @throws core::RangeError If the transmitter or receiver is too close to the target for accurate simulation.
	 * @throws std::runtime_error If no time points are available for execution.
	 */
	void simulateTarget(const Transmitter* trans, Receiver* recv, const Target* targ,
	                    const TransmitterPulse* signal)
	{
		// Convert time-related values to std::chrono::duration for improved type safety
		const auto start_time = std::chrono::duration<RealType>(signal->time);
		const auto end_time = start_time + std::chrono::duration<RealType>(signal->wave->getLength());
		const auto sample_time = std::chrono::duration<RealType>(1.0 / params::cwSampleRate());
		const int point_count = static_cast<int>(std::ceil(signal->wave->getLength() / sample_time.count()));

		// Create a response object for this signal
		auto response = std::make_unique<Response>(signal->wave, trans);

		try
		{
			if (point_count == 0)
			{
				LOG(Level::FATAL, "No time points are available for execution!");
				throw std::runtime_error("No time points are available for execution!");
			}
			// Loop over all time points, including the final one
			for (int i = 0; i <= point_count; ++i)
			{
				// Calculate the current time
				const auto current_time = i < point_count
					                          ? start_time + i * sample_time
					                          : end_time;

				// Simulate the results for each time step
				core::ReResults results{};
				solveRe(trans, recv, targ, current_time, sample_time, signal->wave, results);

				// Adding interpolation point with designated initializers
				interp::InterpPoint point{
					.power = results.power,
					.time = current_time.count() + results.delay,
					.delay = results.delay,
					.doppler = results.doppler,
					.phase = results.phase,
					.noise_temperature = results.noise_temperature
				};
				response->addInterpPoint(point);
			}
		}
		catch (const core::RangeError&)
		{
			LOG(Level::FATAL, "Receiver or Transmitter too close to Target for accurate simulation");
			throw core::RangeError();
		}

		// Add the response to the receiver
		recv->addResponse(std::move(response));
	}

	/**
	 * @brief Simulates the radar interaction between a transmitter-receiver pair.
	 *
	 * This function runs radar simulations for all targets in the simulation world,
	 * as well as for direct interactions between the transmitter and receiver.
	 * It handles multiple pulses and adds the resulting responses to the receiver.
	 *
	 * @param trans Pointer to the radar transmitter.
	 * @param recv Pointer to the radar receiver.
	 * @param world Pointer to the simulation environment.
	 */
	void simulatePair(const Transmitter* trans, Receiver* recv, const core::World* world)
	{
		constexpr auto flag_nodirect = Receiver::RecvFlag::FLAG_NODIRECT;

		// Use stack allocation instead of dynamic memory for pulse
		TransmitterPulse pulse{};

		// Get the pulse counts
		const int pulses = trans->getPulseCount();

		// Loop through all pulses
		for (int i = 0; i < pulses; ++i)
		{
			// Set the pulse
			trans->setPulse(&pulse, i);

			// Simulate target interactions for each target in the world
			std::ranges::for_each(world->getTargets(),
			                      [&](const auto& target) { simulateTarget(trans, recv, target.get(), &pulse); });

			// Add direct response unless the flag FLAG_NODIRECT is set
			if (!recv->checkFlag(flag_nodirect)) { addDirect(trans, recv, &pulse); }
		}
	}
}

namespace core
{
	void runThreadedSim(const World* world, pool::ThreadPool& pool)
	{
		// Get the receivers and transmitters from the world
		const auto& receivers = world->getReceivers();
		const auto& transmitters = world->getTransmitters();

		LOG(Level::INFO, "Running radar simulation for {} receivers", receivers.size());
		// Enqueue tasks for simulating receiver-transmitter pairs
		for (const auto& receiver : receivers)
		{
			for (const auto& transmitter : transmitters)
			{
				pool.enqueue([&transmitter, &receiver, world]
				{
					simulatePair(transmitter.get(), receiver.get(), world);
				});
			}
		}

		pool.wait();

		for (const auto& receiver : receivers)
		{
			LOG(Level::DEBUG, "{} responses added to '{}'", receiver->getResponseCount(), receiver->getName());
		}

		LOG(Level::INFO, "Rendering responses for {} receivers", receivers.size());
		// Enqueue tasks for rendering each receiver
		for (const auto& receiver : receivers) { pool.enqueue([&receiver, &pool] { receiver->render(pool); }); }

		pool.wait();
	}
}
