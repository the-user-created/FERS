// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
 * @file channel_model.h
 * @brief Header for radar channel propagation and interaction models.
 *
 * This file contains the declarations for functions that model the radar channel,
 * including direct and reflected signal path calculations, Doppler effects, and
 * power scaling according to the radar range equation. These functions form the
 * core physics engine for determining the properties of a received signal based
 * on the positions and velocities of transmitters, receivers, and targets.
 */

#pragma once

#include <chrono>
#include <exception>
#include <memory>

#include "core/config.h"

namespace radar
{
	class Receiver;

	class Transmitter;

	class Target;
}

namespace serial
{
	class Response;
}

namespace fers_signal
{
	class RadarSignal;
}

namespace simulation
{
	/**
	 * @struct ReResults
	 * @brief Stores the intermediate results of a radar equation calculation for a single time point.
	 */
	struct ReResults
	{
		RealType power; /**< Power scaling factor (dimensionless, relative to transmitted power). */
		RealType delay; /**< Signal propagation delay in seconds. */
		RealType doppler_factor; /**< Relativistic Doppler factor of the radar signal (f_recv / f_trans). */
		RealType phase; /**< Phase shift in radians due to propagation delay. */
		RealType noise_temperature; /**< Noise temperature contribution from the receiver's perspective. */
	};

	/**
	 * @class RangeError
	 * @brief Exception thrown when a range calculation fails, typically due to objects being too close.
	 */
	class RangeError final : public std::exception
	{
	public:
		/**
		 * @brief Provides the error message for the exception.
		 * @return A C-style string describing the error.
		 */
		[[nodiscard]] const char* what() const noexcept override
		{
			return "Range error in radar equation calculations";
		}
	};

	/**
	 * @brief Solves the bistatic radar equation for a reflected path (Tx -> Tgt -> Rx).
	 *
	 * This function calculates the signal properties (power, delay, Doppler, phase, noise)
	 * for a signal traveling from a transmitter, reflecting off a target, and arriving at a receiver.
	 * It accounts for antenna gains, target RCS, propagation loss, and relativistic Doppler effects.
	 *
	 * @param trans Pointer to the transmitter.
	 * @param recv Pointer to the receiver.
	 * @param targ Pointer to the target.
	 * @param time The time at which the pulse is transmitted.
	 * @param length The duration of the pulse segment, used for velocity calculation.
	 * @param wave Pointer to the transmitted radar signal.
	 * @param results Output struct to store the calculation results.
	 * @throws RangeError If the target is too close to the transmitter or receiver.
	 */
	void solveRe(const radar::Transmitter* trans, const radar::Receiver* recv, const radar::Target* targ,
				 const std::chrono::duration<RealType>& time, const std::chrono::duration<RealType>& length,
				 const fers_signal::RadarSignal* wave, ReResults& results);

	/**
	 * @brief Solves the radar equation for a direct path (Tx -> Rx).
	 *
	 * This function calculates the signal properties for a direct line-of-sight signal
	 * traveling from a transmitter to a receiver.
	 *
	 * @param trans Pointer to the transmitter.
	 * @param recv Pointer to the receiver.
	 * @param time The time at which the pulse is transmitted.
	 * @param length The duration of the pulse segment, used for velocity calculation.
	 * @param wave Pointer to the transmitted radar signal.
	 * @param results Output struct to store the calculation results.
	 * @throws RangeError If the transmitter and receiver are too close.
	 */
	void solveReDirect(const radar::Transmitter* trans, const radar::Receiver* recv,
					   const std::chrono::duration<RealType>& time, const std::chrono::duration<RealType>& length,
					   const fers_signal::RadarSignal* wave, ReResults& results);

	/**
	 * @brief Calculates the complex envelope contribution for a direct propagation path (Tx -> Rx) at a specific time.
	 * This function is used for Continuous Wave (CW) simulations.
	 *
	 * @param trans The transmitter.
	 * @param recv The receiver.
	 * @param timeK The current simulation time.
	 * @return The complex I/Q sample contribution for this path.
	 */
	ComplexType calculateDirectPathContribution(const radar::Transmitter* trans, const radar::Receiver* recv,
												RealType timeK);

	/**
	 * @brief Calculates the complex envelope contribution for a reflected path (Tx -> Tgt -> Rx) at a specific time.
	 * This function is used for Continuous Wave (CW) simulations.
	 *
	 * @param trans The transmitter.
	 * @param recv The receiver.
	 * @param targ The target.
	 * @param timeK The current simulation time.
	 * @return The complex I/Q sample contribution for this path.
	 */
	ComplexType calculateReflectedPathContribution(const radar::Transmitter* trans, const radar::Receiver* recv,
												   const radar::Target* targ, RealType timeK);

	/**
	 * @brief Creates a Response object by simulating a signal's interaction over its duration.
	 *
	 * This function iterates over the duration of a transmitted pulse, calling the
	 * appropriate channel model function (`solveRe` or `solveReDirect`) at discrete
	 * time steps to generate a series of `InterpPoint`s. These points capture the
	 * time-varying properties of the received signal and are collected into a `Response` object.
	 *
	 * @param trans Pointer to the transmitter.
	 * @param recv Pointer to the receiver.
	 * @param signal Pointer to the transmitted pulse signal.
	 * @param startTime The absolute simulation time when the pulse transmission starts.
	 * @param targ Optional pointer to a target. If null, a direct path is simulated.
	 * @return A unique pointer to the generated Response object.
	 * @throws RangeError If the channel model reports an invalid geometry.
	 * @throws std::runtime_error If the simulation parameters result in zero time steps.
	 */
	std::unique_ptr<serial::Response> calculateResponse(const radar::Transmitter* trans, const radar::Receiver* recv,
														const fers_signal::RadarSignal* signal, RealType startTime,
														const radar::Target* targ = nullptr);
}
