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
#include "timing/timing.h"

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
	 * @brief Calculates the complex envelope contribution for a direct propagation path (Tx -> Rx) at a specific time.
	 * @param trans The transmitter.
	 * @param recv The receiver.
	 * @param timeK The current simulation time.
	 * @return The complex I/Q sample contribution for this path.
	 */
	ComplexType calculateDirectPathContribution(const Transmitter* trans, const Receiver* recv, const RealType timeK)
	{
		const auto p_tx = trans->getPlatform()->getPosition(timeK);
		const auto p_rx = recv->getPlatform()->getPosition(timeK);

		const auto tx_to_rx_vec = p_rx - p_tx;
		const auto range = tx_to_rx_vec.length();

		if (range <= EPSILON) { return {0.0, 0.0}; }

		const auto u_ji = tx_to_rx_vec / range;
		const RealType tau = range / params::c();
		const auto signal = trans->getSignal();
		const RealType carrier_freq = signal->getCarrier();
		const RealType lambda = params::c() / carrier_freq;

		// Amplitude Scaling (Friis Transmission Equation)
		const RealType tx_gain = trans->getGain(SVec3(u_ji), trans->getRotation(timeK), lambda);
		const RealType rx_gain = recv->getGain(SVec3(-u_ji), recv->getRotation(timeK + tau), lambda);

		RealType power_scaling = signal->getPower() * tx_gain * rx_gain * lambda * lambda / (std::pow(4 * PI, 2) *
			range * range);
		if (recv->checkFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS))
		{
			power_scaling = signal->getPower() * tx_gain * rx_gain * lambda * lambda / std::pow(4 * PI, 2);
		}
		const RealType amplitude = std::sqrt(power_scaling);

		// Complex Envelope Contribution
		const RealType phase = -2 * PI * carrier_freq * tau;
		ComplexType contribution = std::polar(amplitude, phase);

		// Non-coherent Local Oscillator Effects
		const auto tx_timing = trans->getTiming();
		const auto rx_timing = recv->getTiming();
		const RealType delta_f = tx_timing->getFreqOffset() - rx_timing->getFreqOffset();
		const RealType delta_phi = tx_timing->getPhaseOffset() - rx_timing->getPhaseOffset();
		const RealType non_coherent_phase = 2 * PI * delta_f * timeK + delta_phi;
		contribution *= std::polar(1.0, non_coherent_phase);

		return contribution;
	}

	/**
	 * @brief Calculates the complex envelope contribution for a reflected path (Tx -> Tgt -> Rx) at a specific time.
	 * @param trans The transmitter.
	 * @param recv The receiver.
	 * @param targ The target.
	 * @param timeK The current simulation time.
	 * @return The complex I/Q sample contribution for this path.
	 */
	ComplexType calculateReflectedPathContribution(const Transmitter* trans, const Receiver* recv, const Target* targ,
	                                               const RealType timeK)
	{
		const auto p_tx = trans->getPlatform()->getPosition(timeK);
		const auto p_rx = recv->getPlatform()->getPosition(timeK);
		const auto p_tgt = targ->getPlatform()->getPosition(timeK);

		const auto tx_to_tgt_vec = p_tgt - p_tx;
		const auto tgt_to_rx_vec = p_rx - p_tgt;
		const RealType r_jm = tx_to_tgt_vec.length();
		const RealType r_mi = tgt_to_rx_vec.length();

		if (r_jm <= EPSILON || r_mi <= EPSILON) { return {0.0, 0.0}; }

		const auto u_jm = tx_to_tgt_vec / r_jm;
		const auto u_mi = tgt_to_rx_vec / r_mi;

		const RealType tau = (r_jm + r_mi) / params::c();
		const auto signal = trans->getSignal();
		const RealType carrier_freq = signal->getCarrier();
		const RealType lambda = params::c() / carrier_freq;

		// Amplitude Scaling (Bistatic Radar Range Equation)
		SVec3 in_angle_sv(u_jm);
		SVec3 out_angle_sv(-u_mi);
		const RealType rcs = targ->getRcs(in_angle_sv, out_angle_sv);
		const RealType tx_gain = trans->getGain(SVec3(u_jm), trans->getRotation(timeK), lambda);
		const RealType rx_gain = recv->getGain(SVec3(-u_mi), recv->getRotation(timeK + tau), lambda);

		RealType power_scaling = signal->getPower() * tx_gain * rx_gain * rcs * lambda * lambda / (std::pow(
			4 * PI, 3) * r_jm * r_jm * r_mi * r_mi);
		if (recv->checkFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS))
		{
			power_scaling = signal->getPower() * tx_gain * rx_gain * rcs * lambda * lambda / std::pow(4 * PI, 3);
		}
		const RealType amplitude = std::sqrt(power_scaling);

		// Complex Envelope Contribution
		const RealType phase = -2 * PI * carrier_freq * tau;
		ComplexType contribution = std::polar(amplitude, phase);

		// Non-coherent Local Oscillator Effects
		const auto tx_timing = trans->getTiming();
		const auto rx_timing = recv->getTiming();
		const RealType delta_f = tx_timing->getFreqOffset() - rx_timing->getFreqOffset();
		const RealType delta_phi = tx_timing->getPhaseOffset() - rx_timing->getPhaseOffset();
		const RealType non_coherent_phase = 2 * PI * delta_f * timeK + delta_phi;
		contribution *= std::polar(1.0, non_coherent_phase);

		return contribution;
	}

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

		const auto trpos_end = SVec3(
			trans->getPosition(time.count() + length.count()) - recv->getPosition(time.count() + length.count()));
		const RealType r_end = trpos_end.length;
		const RealType doppler_shift = (r_end - distance) / length.count();

		results.doppler = (params::c() + doppler_shift) / (params::c() - doppler_shift);

		// Phase calculation, ensuring the phase is wrapped within [0, 2π]
		results.phase = std::fmod(results.delay * 2 * PI * wave->getCarrier(), 2 * PI);

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
		const auto sample_time = std::chrono::duration<RealType>(1.0 / params::pathSamplingRate());
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
					.doppler = results.doppler,
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

	void runThreadedCwSim(const World* world, pool::ThreadPool& pool)
	{
		const auto& receivers = world->getReceivers();
		const auto& transmitters = world->getTransmitters();
		const auto& targets = world->getTargets();

		const RealType start_time = params::startTime();
		const RealType end_time = params::endTime();
		const RealType dt = 1.0 / params::rate(); // TODO: No oversampling of the data yet
		const auto num_samples = static_cast<size_t>(std::ceil((end_time - start_time) / dt));

		LOG(Level::INFO, "Running CW simulation for {} receivers over {} samples", receivers.size(), num_samples);

		// Pre-allocate memory for I/Q data in each receiver
		for (const auto& receiver : receivers)
		{
			receiver->prepareCwData(num_samples);
		}

		size_t sample_index = 0;
		for (RealType t_k = start_time; t_k < end_time; t_k += dt)
		{
			if (sample_index >= num_samples) { continue; }

			pool.enqueue([&, t_k, sample_index]
			{
				for (const auto& receiver : receivers)
				{
					ComplexType total_sample{0.0, 0.0};

					for (const auto& transmitter : transmitters)
					{
						// Direct Path
						if (!receiver->checkFlag(Receiver::RecvFlag::FLAG_NODIRECT))
						{
							total_sample += calculateDirectPathContribution(transmitter.get(), receiver.get(), t_k);
						}

						// Reflected Paths
						for (const auto& target : targets)
						{
							total_sample += calculateReflectedPathContribution(
								transmitter.get(), receiver.get(),
								target.get(), t_k);
						}
					}
					receiver->setCwSample(sample_index, total_sample);
				}
			});
			++sample_index;
		}
		pool.wait();

		LOG(Level::INFO, "Rendering CW data for {} receivers", receivers.size());
		for (const auto& receiver : receivers)
		{
			pool.enqueue([&receiver, &pool] { receiver->render(pool); });
		}
		pool.wait();
	}
}
