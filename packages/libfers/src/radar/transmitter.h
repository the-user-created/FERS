// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
 * @file transmitter.h
 * @brief Header file for the Transmitter class in the radar namespace.
 */

#pragma once

#include <optional>

#include "radar_obj.h"

namespace fers_signal
{
	class RadarSignal;
}

namespace radar
{
	/**
	 * @struct TransmitterPulse
	 * @brief Struct representing a radar pulse emitted by the transmitter.
	 *
	 */
	struct TransmitterPulse
	{
		fers_signal::RadarSignal* wave; ///< Pointer to the radar signal wave.

		RealType time; ///< Time at which the pulse is emitted.
	};

	/**
	 * @struct SchedulePeriod
	 * @brief Represents a time period during which the transmitter is active.
	 */
	struct SchedulePeriod
	{
		RealType start;
		RealType end;
	};


	/**
	 * @class Transmitter
	 * @brief Represents a radar transmitter system.
	 */
	class Transmitter final : public Radar
	{
	public:
		/**
		 * @brief Constructor for the Transmitter class.
		 *
		 * @param platform Pointer to the platform object.
		 * @param name Name of the transmitter.
		 * @param mode The operational mode (PULSED_MODE or CW_MODE).
		 */
		Transmitter(Platform* platform, std::string name, const OperationMode mode) noexcept :
			Radar(platform, std::move(name)), _mode(mode)
		{
		}

		~Transmitter() override = default;

		Transmitter(const Transmitter&) = delete;

		Transmitter& operator=(const Transmitter&) = delete;

		Transmitter(Transmitter&&) = delete;

		Transmitter& operator=(Transmitter&&) = delete;

		/**
		 * @brief Retrieves the pulse repetition frequency (PRF).
		 *
		 * @return The current PRF of the transmitter.
		 */
		[[nodiscard]] RealType getPrf() const noexcept { return _prf; }

		/**
		 * @brief Retrieves the radar signal currently being transmitted.
		 *
		 * @return Pointer to the RadarSignal object being transmitted.
		 */
		[[nodiscard]] fers_signal::RadarSignal* getSignal() const noexcept { return _signal; }

		/**
		 * @brief Gets the operational mode of the transmitter.
		 *
		 * @return The operational mode (PULSED_MODE or CW_MODE).
		 */
		[[nodiscard]] OperationMode getMode() const noexcept { return _mode; }

		/**
		 * @brief Sets the radar signal wave to be transmitted.
		 *
		 * @param pulse Pointer to the RadarSignal object representing the wave.
		 */
		void setWave(fers_signal::RadarSignal* pulse) noexcept { _signal = pulse; }

		/**
		 * @brief Sets the radar signal wave to be transmitted.
		 *
		 * @param signal Pointer to the RadarSignal object to be transmitted.
		 */
		void setSignal(fers_signal::RadarSignal* signal) noexcept { _signal = signal; }

		/**
		 * @brief Assigns a pulse to be transmitted at a given time.
		 *
		 * @param pulse Pointer to the TransmitterPulse structure to hold the pulse data.
		 * @param number Pulse number for which to set the time and waveform.
		 * @throws std::logic_error If the transmitter is not associated with a timing source.
		 */
		void setPulse(TransmitterPulse* pulse, int number) const;

		/**
		 * @brief Sets the pulse repetition frequency (PRF) of the transmitter.
		 *
		 * @param mprf Desired PRF to be set.
		 */
		void setPrf(RealType mprf) noexcept;

		/**
		 * @brief Adds an active period to the transmitter's schedule.
		 * @param start Start time of the active period.
		 * @param end End time of the active period.
		 */
		void addSchedulePeriod(RealType start, RealType end);

		/**
		 * @brief Retrieves the list of active transmission periods.
		 * @return A const reference to the schedule vector.
		 */
		[[nodiscard]] const std::vector<SchedulePeriod>& getSchedule() const noexcept { return _schedule; }

		/**
		 * @brief Determines the valid simulation time for a pulse at or after the given time.
		 *
		 * If the requested time falls within an active period, it is returned.
		 * If it falls in a gap between periods, the start of the next period is returned.
		 * If it falls after all periods, std::nullopt is returned.
		 * If no schedule is defined, the transmitter is considered "always on".
		 *
		 * @param time The proposed pulse time.
		 * @return The actual pulse time, or nullopt if no valid time exists.
		 */
		[[nodiscard]] std::optional<RealType> getNextPulseTime(RealType time) const;

	private:
		fers_signal::RadarSignal* _signal = nullptr; ///< Pointer to the radar signal being transmitted.

		RealType _prf = {}; ///< The pulse repetition frequency (PRF) of the transmitter.

		OperationMode _mode; ///< The operational mode of the transmitter.
		std::vector<SchedulePeriod> _schedule; ///< The schedule of active periods.
	};
}
