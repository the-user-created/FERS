/**
* @file sim_threading.cpp
* @brief Thread management for the simulator.
*
* @authors David Young, Marc Brooker
* @date 2006-07-19
*/

#include "sim_threading.h"

#include <algorithm>
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

namespace
{
	/**
	 * @brief Performs radar simulation for a given transmitter, receiver, and target.
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

		if (!recv->checkFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS))
		{
			const RealType distance_product = transmitter_to_target_distance * receiver_to_target_distance;
			results.power *= std::pow(wavelength, 2) / (std::pow(4 * PI, 2) * std::pow(
				distance_product, 2));
		}

		results.phase = -results.delay * 2 * PI * wave->getCarrier();

		// Relativistic Doppler Calculation
		const RealType dt = length.count();
		const auto c = params::c();

		// Calculate velocities
		const math::Vec3 trans_vel = (trans->getPosition(time.count() + dt) - transmitter_position) / dt;
		const math::Vec3 recv_vel = (recv->getPosition(time.count() + dt) - receiver_position) / dt;
		const math::Vec3 targ_vel = (targ->getPosition(time.count() + dt) - target_position) / dt;

		// Propagation unit vectors
		const math::Vec3 u_ttgt{transmitter_to_target_vector}; // T -> Tgt
		const math::Vec3 u_tgtr = math::Vec3{receiver_to_target_vector} * -1.0; // Tgt -> R

		// Beta vectors
		const math::Vec3 beta_t = trans_vel / c;
		const math::Vec3 beta_r = recv_vel / c;
		const math::Vec3 beta_tgt = targ_vel / c;

		// Lorentz factors (only need T and R, as Tgt cancels)
		const RealType gamma_t = 1.0 / std::sqrt(1.0 - dotProduct(beta_t, beta_t));
		const RealType gamma_r = 1.0 / std::sqrt(1.0 - dotProduct(beta_r, beta_r));

		// Numerators and denominators for the Doppler factor formula
		const RealType term1_num = 1.0 - dotProduct(beta_tgt, u_ttgt);
		const RealType term1_den = 1.0 - dotProduct(beta_t, u_ttgt);
		const RealType term2_num = 1.0 - dotProduct(beta_r, u_tgtr);
		const RealType term2_den = 1.0 - dotProduct(beta_tgt, u_tgtr);

		results.doppler_factor = (term1_num / term1_den) * (term2_num / term2_den) * (gamma_r / gamma_t);

		results.noise_temperature = recv->getNoiseTemperature(recv->getRotation(time.count() + results.delay));
	}

	/**
	 * @brief Performs direct radar simulation between a transmitter and receiver without a target.
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
		const auto tpos = trans->getPosition(time.count());
		const auto rpos = recv->getPosition(time.count());

		const SVec3 transvec{tpos - rpos};
		const RealType distance = transvec.length;

		if (distance <= EPSILON)
		{
			LOG(Level::FATAL, "Transmitter or Receiver too close to Target for accurate simulation");
			throw core::RangeError();
		}

		results.delay = distance / params::c();

		const RealType wavelength = params::c() / wave->getCarrier();
		const RealType transmitter_gain = trans->getGain(transvec, trans->getRotation(time.count()), wavelength);
		const RealType receiver_gain = recv->getGain(SVec3(rpos - tpos),
		                                             recv->getRotation(time.count() + results.delay),
		                                             wavelength);

		results.power = transmitter_gain * receiver_gain * wavelength * wavelength / (4 * PI);

		if (!recv->checkFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS)) { results.power /= 4 * PI * distance * distance; }

		// Relativistic Doppler Calculation
		const RealType dt = length.count();
		const auto c = params::c();

		// Calculate velocities
		const math::Vec3 trans_vel = (trans->getPosition(time.count() + dt) - tpos) / dt;
		const math::Vec3 recv_vel = (recv->getPosition(time.count() + dt) - rpos) / dt;

		// Propagation unit vector (Transmitter to Receiver)
		math::Vec3 u_tr = rpos - tpos;
		u_tr /= distance;

		// Beta vectors
		const math::Vec3 beta_t = trans_vel / c;
		const math::Vec3 beta_r = recv_vel / c;

		// Lorentz factors
		const RealType gamma_t = 1.0 / std::sqrt(1.0 - dotProduct(beta_t, beta_t));
		const RealType gamma_r = 1.0 / std::sqrt(1.0 - dotProduct(beta_r, beta_r));

		// Numerators and denominators for the Doppler factor formula
		const RealType num = 1.0 - dotProduct(beta_r, u_tr);
		const RealType den = 1.0 - dotProduct(beta_t, u_tr);

		results.doppler_factor = (num / den) * (gamma_r / gamma_t);

		results.phase = -results.delay * 2 * PI * wave->getCarrier();

		results.noise_temperature = recv->getNoiseTemperature(recv->getRotation(time.count() + results.delay));
	}

	/**
	 * @brief Simulates the radar signal interaction with a target or performs direct simulation.
	 *
	 * @param trans Pointer to the radar transmitter.
	 * @param recv Pointer to the radar receiver.
	 * @param signal Pointer to the radar signal pulse being simulated.
	 * @param targ Pointer to the radar target.
	 * @throws core::RangeError If the transmitter or receiver is too close to the target for accurate simulation.
	 * @throws std::runtime_error If no time points are available for execution.
	 */
	void simulateResponse(const Transmitter* trans, Receiver* recv, const TransmitterPulse* signal,
	                      const Target* targ = nullptr)
	{
		if (targ == nullptr && trans->getAttached() == recv) { return; }

		const auto start_time = std::chrono::duration<RealType>(signal->time);
		const auto end_time = start_time + std::chrono::duration<RealType>(signal->wave->getLength());
		const auto sample_time = std::chrono::duration<RealType>(1.0 / params::simSamplingRate());
		const int point_count = static_cast<int>(std::ceil(signal->wave->getLength() / sample_time.count()));

		// Check for a valid point count in case of target simulation
		if (targ && point_count == 0)
		{
			LOG(Level::FATAL, "No time points are available for execution!");
			throw std::runtime_error("No time points are available for execution!");
		}

		auto response = std::make_unique<Response>(signal->wave, trans);

		try
		{
			for (int i = 0; i <= point_count; ++i)
			{
				const auto current_time = i < point_count ? start_time + i * sample_time : end_time;

				core::ReResults results{};
				// If a target is provided, use target simulation; otherwise, direct simulation.
				if (targ)
				{
					solveRe(trans, recv, targ, current_time, sample_time, signal->wave, results);
				}
				else
				{
					solveReDirect(trans, recv, current_time, sample_time, signal->wave, results);
				}

				interp::InterpPoint point{
					.power = results.power,
					.time = current_time.count() + results.delay,
					.delay = results.delay,
					.doppler_factor = results.doppler_factor,
					.phase = results.phase,
					.noise_temperature = results.noise_temperature
				};
				response->addInterpPoint(point);
			}
		}
		catch (const core::RangeError&)
		{
			LOG(Level::FATAL, "Receiver or Transmitter too close for accurate simulation");
			throw core::RangeError();
		}

		recv->addResponse(std::move(response));
	}

	/**
	 * @brief Simulates the radar interaction between a transmitter-receiver pair.
	 *
	 * @param trans Pointer to the radar transmitter.
	 * @param recv Pointer to the radar receiver.
	 * @param world Pointer to the simulation environment.
	 */
	void simulatePair(const Transmitter* trans, Receiver* recv, const core::World* world)
	{
		constexpr auto flag_nodirect = Receiver::RecvFlag::FLAG_NODIRECT;

		TransmitterPulse pulse{};

		const int pulses = trans->getPulseCount();

		for (int i = 0; i < pulses; ++i)
		{
			trans->setPulse(&pulse, i);

			std::ranges::for_each(world->getTargets(), [&](const auto& target)
			{
				simulateResponse(trans, recv, &pulse, target.get());
			});

			if (!recv->checkFlag(flag_nodirect)) { simulateResponse(trans, recv, &pulse); }
		}
	}
}

namespace core
{
	void runThreadedSim(const World* world, pool::ThreadPool& pool)
	{
		const auto& receivers = world->getReceivers();
		const auto& transmitters = world->getTransmitters();

		LOG(Level::INFO, "Running radar simulation for {} receivers", receivers.size());
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
		for (const auto& receiver : receivers) { pool.enqueue([&receiver, &pool] { receiver->render(pool); }); }

		pool.wait();
	}
}
