/**
* @file prototype_timing.h
* @brief Header file for the PrototypeTiming class.
*
* This file contains the declaration of the PrototypeTiming class, which is responsible for managing timing parameters
* such as frequency, phase, and frequency offsets, as well as handling alpha and weight vectors.
* It also includes methods for handling both constant and random offsets for frequency and phase.
* The class is used in the timing module of the application.
*
* @authors David Young, Marc Brooker
* @date 2024-09-17
*/

#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "config.h"

namespace timing
{
	/**
	* @class PrototypeTiming
	* @brief Manages timing properties such as frequency, offsets, and synchronization.
	*
	* The PrototypeTiming class provides an interface to manage timing properties, including setting and getting
	* frequency, phase offsets, and synchronizing pulses. It supports both constant and random offsets for frequency
	* and phase, and tracks alpha and weight values for some calculations. The class includes methods to set and copy
	* these alphas and weights for other purposes.
	*/
	class PrototypeTiming
	{
	public:
		/**
		* @brief Constructor for PrototypeTiming.
		*
		* Initializes a PrototypeTiming object with the given name.
		*
		* @param name The name of the timing source.
		*/
		explicit PrototypeTiming(std::string name) noexcept : _name(std::move(name)) {}

		/**
		* @brief Default destructor for PrototypeTiming.
		*/
		~PrototypeTiming() = default;

		/**
		* @brief Copies the alphas and weights vectors.
		*
		* Copies the stored alpha and weight values to the provided vectors.
		*
		* @param alphas Reference to the vector where alpha values will be copied.
		* @param weights Reference to the vector where weight values will be copied.
		*/
		void copyAlphas(std::vector<RealType>& alphas, std::vector<RealType>& weights) const noexcept;

		/**
		* @brief Gets the current frequency.
		*
		* Returns the frequency value set in the PrototypeTiming object.
		*
		* @return The current frequency value.
		*/
		[[nodiscard]] RealType getFrequency() const noexcept { return _frequency; }

		/**
		* @brief Gets the name of the timing source.
		*
		* Returns the name of the PrototypeTiming instance.
		*
		* @return The name of the timing source.
		*/
		[[nodiscard]] std::string getName() const { return _name; }

		/**
		* @brief Checks if synchronization on pulse is enabled.
		*
		* Returns whether the timing source is set to synchronize on pulse.
		*
		* @return True if synchronization on pulse is enabled, false otherwise.
		*/
		[[nodiscard]] bool getSyncOnPulse() const noexcept { return _sync_on_pulse; }

		/**
		* @brief Gets the phase offset.
		*
		* Returns the phase offset if set, otherwise returns 0. Supports random phase offset generation.
		*
		* @return The phase offset value.
		*/
		[[nodiscard]] RealType getPhaseOffset() const noexcept;

		/**
		* @brief Gets the frequency offset.
		*
		* Returns the frequency offset if set, otherwise returns 0. Supports random frequency offset generation.
		*
		* @return The frequency offset value.
		*/
		[[nodiscard]] RealType getFreqOffset() const noexcept;

		/**
		* @brief Sets the frequency value.
		*
		* Sets the frequency for the timing source.
		*
		* @param freq The frequency value to be set.
		*/
		void setFrequency(const RealType freq) noexcept { _frequency = freq; }

		/**
		* @brief Enables synchronization on pulse.
		*
		* Sets the synchronization flag to true, indicating the timing source will synchronize on pulse.
		*/
		void setSyncOnPulse() noexcept { _sync_on_pulse = true; }

		/**
		* @brief Sets an alpha and weight value.
		*
		* Adds the specified alpha and weight values to their respective vectors.
		*
		* @param alpha The alpha value to be added.
		* @param weight The weight value to be added.
		*/
		void setAlpha(RealType alpha, RealType weight) noexcept;

		/**
		* @brief Sets the frequency offset.
		*
		* Sets a constant frequency offset.
		* Logs a conflict if a random frequency offset is already set.
		*
		* @param offset The frequency offset to be set.
		*/
		void setFreqOffset(RealType offset) noexcept;

		/**
		* @brief Sets the phase offset.
		*
		* Sets a constant phase offset. Logs a conflict if a random phase offset is already set.
		*
		* @param offset The phase offset to be set.
		*/
		void setPhaseOffset(RealType offset) noexcept;

		/**
		* @brief Sets a random frequency offset.
		*
		* Sets a random frequency offset based on the provided standard deviation.
		* Logs a conflict if a constant frequency offset is already set.
		*
		* @param stdev The standard deviation for generating the random frequency offset.
		*/
		void setRandomFreqOffset(RealType stdev) noexcept;

		/**
		* @brief Sets a random phase offset.
		*
		* Sets a random phase offset based on the provided standard deviation. Logs a conflict if a constant phase offset is already set.
		*
		* @param stdev The standard deviation for generating the random phase offset.
		*/
		void setRandomPhaseOffset(RealType stdev) noexcept;

	private:
		std::string _name; ///< The name of the timing source.
		std::vector<RealType> _alphas; ///< Vector of alpha values.
		std::vector<RealType> _weights; ///< Vector of weight values.
		std::optional<RealType> _freq_offset; ///< Constant frequency offset.
		std::optional<RealType> _phase_offset; ///< Constant phase offset.
		std::optional<RealType> _random_phase; ///< Random phase offset.
		std::optional<RealType> _random_freq; ///< Random frequency offset.
		RealType _frequency{0}; ///< The frequency value.
		bool _sync_on_pulse{false}; ///< Flag indicating synchronization on pulse.

		/**
		* @brief Logs a conflict between random and constant offsets.
		*
		* Logs an error if both random and constant offsets for the same type (frequency or phase) are set.
		*
		* @param offsetType The type of offset ("frequency" or "phase") that caused the conflict.
		*/
		void logOffsetConflict(const std::string& offsetType) const noexcept;
	};
}
