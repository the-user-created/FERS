/**
* @file transmitter.h
* @brief Header file for the Transmitter class in the radar namespace.
*
* @authors David Young, Marc Brooker
* @date 2024-10-07
*/

#pragma once

#include "radar_obj.h"

namespace signal
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
		signal::RadarSignal* wave; ///< Pointer to the radar signal wave.

		RealType time; ///< Time at which the pulse is emitted.
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
		* @param pulsed Boolean indicating whether the transmitter uses pulsed signals.
		*/
		Transmitter(Platform* platform, std::string name, const bool pulsed) noexcept :
			Radar(platform, std::move(name)), _pulsed(pulsed) {}

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
		[[nodiscard]] signal::RadarSignal* getSignal() const noexcept { return _signal; }

		/**
		* @brief Checks if the transmitter is pulsed.
		*
		* @return True if the transmitter is pulsed, false for CW (Continuous Wave) systems.
		*/
		[[nodiscard]] bool getPulsed() const noexcept { return _pulsed; }

		/**
		* @brief Retrieves the pulse count over the transmission duration.
		*
		* @return Number of pulses transmitted.
		*/
		[[nodiscard]] int getPulseCount() const noexcept;

		/**
		* @brief Sets the radar signal wave to be transmitted.
		*
		* @param pulse Pointer to the RadarSignal object representing the wave.
		*/
		void setWave(signal::RadarSignal* pulse) noexcept { _signal = pulse; }

		/**
		* @brief Sets the radar signal wave to be transmitted.
		*
		* @param signal Pointer to the RadarSignal object to be transmitted.
		*/
		void setSignal(signal::RadarSignal* signal) noexcept { _signal = signal; }

		/**
		* @brief Sets whether the transmitter is pulsed or continuous wave.
		*
		* @param pulsed Boolean indicating whether the transmitter is pulsed.
		*/
		void setPulsed(const bool pulsed) noexcept { _pulsed = pulsed; }

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

	private:
		signal::RadarSignal* _signal = nullptr; ///< Pointer to the radar signal being transmitted.

		RealType _prf = {}; ///< The pulse repetition frequency (PRF) of the transmitter.

		bool _pulsed; ///< Boolean indicating whether the transmitter uses pulsed signals.
	};
}
