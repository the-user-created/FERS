/**
* @file transmitter.h
* @brief Header file for the Transmitter class in the radar namespace.
*
* This file defines the structure and operations of the Transmitter class, which inherits from the Radar class.
* The Transmitter class handles radar transmission, pulse modulation,
* and supports both pulsed and continuous wave (CW) radar systems.
* It also manages transmitter pulses, PRF (pulse repetition frequency),
* and interactions with dual transmitters for complex scenarios.
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
	* Contains a radar signal and the time at which the pulse is emitted.
	*/
	struct TransmitterPulse
	{
		/**
		* @brief Pointer to the radar signal wave.
		*/
		signal::RadarSignal* wave;

		/**
		* @brief Time at which the pulse is emitted.
		*/
		RealType time;
	};

	/**
	* @class Transmitter
	* @brief Represents a radar transmitter system.
	*
	* The Transmitter class is responsible for handling radar pulse transmission, including modulation
	* of pulsed and continuous wave (CW) radar signals. It can interact with dual transmitters,
	* control pulse repetition frequency (PRF), and manage radar signal waves.
	*/
	class Transmitter final : public Radar
	{
	public:
		/**
		* @brief Constructor for the Transmitter class.
		*
		* Initializes a Transmitter object with a platform, name, and a boolean indicating if the transmitter is pulsed.
		*
		* @param platform Pointer to the platform object.
		* @param name Name of the transmitter.
		* @param pulsed Boolean indicating whether the transmitter uses pulsed signals.
		*/
		Transmitter(Platform* platform, std::string name, const bool pulsed) noexcept
			: Radar(platform, std::move(name)), _pulsed(pulsed) {}

		/**
		* @brief Default destructor.
		*
		* Cleans up the Transmitter object.
		*/
		~Transmitter() override = default;

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
		* @brief Retrieves the dual transmitter if applicable.
		*
		* @return Pointer to the dual Transmitter object, or nullptr if none is set.
		*/
		[[nodiscard]] Transmitter* getDual() const noexcept { return _dual; }

		/**
		* @brief Checks if the transmitter is pulsed.
		*
		* @return True if the transmitter is pulsed, false for CW (Continuous Wave) systems.
		*/
		[[nodiscard]] bool getPulsed() const noexcept { return _pulsed; }

		/**
		* @brief Retrieves the pulse count over the transmission duration.
		*
		* Computes the number of pulses transmitted based on the time duration and PRF.
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
		* @brief Sets a dual transmitter to enable more complex transmission scenarios.
		*
		* @param dual Pointer to the dual Transmitter object.
		*/
		void setDual(Transmitter* dual) noexcept { _dual = dual; }

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
		* This function sets the time and waveform for a specific pulse number based on the PRF.
		*
		* @param pulse Pointer to the TransmitterPulse structure to hold the pulse data.
		* @param number Pulse number for which to set the time and waveform.
		* @throws std::logic_error If the transmitter is not associated with a timing source.
		*/
		void setPulse(TransmitterPulse* pulse, int number) const;

		/**
		* @brief Sets the pulse repetition frequency (PRF) of the transmitter.
		*
		* Adjusts the PRF based on the system parameters and oversample ratio.
		*
		* @param mprf Desired PRF to be set.
		*/
		void setPrf(RealType mprf) noexcept;

	private:
		/**
		* @brief Pointer to the radar signal being transmitted.
		*/
		signal::RadarSignal* _signal = nullptr;

		/**
		* @brief The pulse repetition frequency (PRF) of the transmitter.
		*/
		RealType _prf = {};

		/**
		* @brief Boolean indicating whether the transmitter uses pulsed signals.
		*/
		bool _pulsed;

		/**
		* @brief Pointer to a dual transmitter for complex transmission scenarios.
		*/
		Transmitter* _dual = nullptr;
	};

	/**
	* @brief Creates a multipath dual transmitter.
	*
	* Generates a dual transmitter that handles multipath radar signals, typically used for simulating complex
	* surface reflections.
	*
	* @param trans Pointer to the primary Transmitter.
	* @param surf Pointer to the multipath surface object.
	* @return Pointer to the newly created dual Transmitter.
	*/
	inline Transmitter* createMultipathDual(Transmitter* trans, const math::MultipathSurface* surf)
	{
		return createMultipathDualBase(trans, surf);
	}
}
