// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
 * @file channel_model.cpp
 * @brief Implementation of radar channel propagation and interaction models.
 *
 * This file provides the implementations for the functions that model the radar
 * channel, as declared in channel_model.h. It contains the core physics calculations
 * that determine signal properties based on geometry, velocity, and object characteristics.
 */

#include "channel_model.h"

#include <cmath>

#include <libfers/geometry_ops.h>
#include <libfers/logging.h>
#include <libfers/parameters.h>
#include <libfers/radar_obj.h>
#include <libfers/receiver.h>
#include <libfers/target.h>
#include <libfers/transmitter.h>
#include "signal/radar_signal.h"
#include "timing/timing.h"

using radar::Transmitter;
using radar::Receiver;
using radar::Target;
using math::SVec3;
using fers_signal::RadarSignal;
using logging::Level;

namespace simulation
{
	void solveRe(const Transmitter* trans, const Receiver* recv, const Target* targ,
	             const std::chrono::duration<RealType>& time, const std::chrono::duration<RealType>& length,
	             const RadarSignal* wave, ReResults& results)
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
			throw RangeError();
		}

		transmitter_to_target_vector.length = 1;
		receiver_to_target_vector.length = 1;

		results.delay = (transmitter_to_target_distance + receiver_to_target_distance) / params::c();

		const auto rcs = targ->getRcs(transmitter_to_target_vector, receiver_to_target_vector, time.count());
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

	void solveReDirect(const Transmitter* trans, const Receiver* recv, const std::chrono::duration<RealType>& time,
	                   const std::chrono::duration<RealType>& length, const RadarSignal* wave, ReResults& results)
	{
		const auto tpos = trans->getPosition(time.count());
		const auto rpos = recv->getPosition(time.count());

		const SVec3 transvec{tpos - rpos};
		const RealType distance = transvec.length;

		if (distance <= EPSILON)
		{
			LOG(Level::FATAL, "Transmitter or Receiver too close to Target for accurate simulation");
			throw RangeError();
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
		const RealType rcs = targ->getRcs(in_angle_sv, out_angle_sv, timeK);
		const RealType tx_gain = trans->getGain(SVec3(u_jm), trans->getRotation(timeK), lambda);
		// TODO: Is this meant to use timeK + tau?
		const RealType rx_gain = recv->getGain(SVec3(-u_mi), recv->getRotation(timeK + tau), lambda);

		RealType power_scaling = signal->getPower() * tx_gain * rx_gain * rcs * lambda * lambda / (std::pow(
			4 * PI, 3) * r_jm * r_jm * r_mi * r_mi);
		if (recv->checkFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS))
		{
			power_scaling = signal->getPower() * tx_gain * rx_gain * rcs * lambda * lambda / std::pow(4 * PI, 3);
		}
		const RealType amplitude = std::sqrt(power_scaling);

		// Complex Envelope Contribution
		// TODO: Use fmod?
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
}
