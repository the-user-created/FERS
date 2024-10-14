/**
* @file prototype_timing.h
* @brief Header file for the PrototypeTiming class.
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
	*/
	class PrototypeTiming
	{
	public:
		/**
		* @brief Constructor for PrototypeTiming.
		*
		* @param name The name of the timing source.
		*/
		explicit PrototypeTiming(std::string name) noexcept : _name(std::move(name)) {}

		~PrototypeTiming() = default;

		/**
		* @brief Copies the alphas and weights vectors.
		*
		* @param alphas Reference to the vector where alpha values will be copied.
		* @param weights Reference to the vector where weight values will be copied.
		*/
		void copyAlphas(std::vector<RealType>& alphas, std::vector<RealType>& weights) const noexcept;

		/**
		* @brief Gets the current frequency.
		*
		* @return The current frequency value.
		*/
		[[nodiscard]] RealType getFrequency() const noexcept { return _frequency; }

		/**
		* @brief Gets the name of the timing source.
		*
		* @return The name of the timing source.
		*/
		[[nodiscard]] std::string getName() const { return _name; }

		/**
		* @brief Checks if synchronization on pulse is enabled.
		*
		* @return True if synchronization on pulse is enabled, false otherwise.
		*/
		[[nodiscard]] bool getSyncOnPulse() const noexcept { return _sync_on_pulse; }

		/**
		* @brief Gets the phase offset.
		*
		* @return The phase offset value.
		*/
		[[nodiscard]] RealType getPhaseOffset() const noexcept;

		/**
		* @brief Gets the frequency offset.
		*
		* @return The frequency offset value.
		*/
		[[nodiscard]] RealType getFreqOffset() const noexcept;

		/**
		* @brief Sets the frequency value.
		*
		* @param freq The frequency value to be set.
		*/
		void setFrequency(const RealType freq) noexcept { _frequency = freq; }

		/**
		* @brief Enables synchronization on pulse.
		*/
		void setSyncOnPulse() noexcept { _sync_on_pulse = true; }

		/**
		* @brief Sets an alpha and weight value.
		*
		* @param alpha The alpha value to be added.
		* @param weight The weight value to be added.
		*/
		void setAlpha(RealType alpha, RealType weight) noexcept;

		/**
		* @brief Sets the frequency offset.
		*
		* @param offset The frequency offset to be set.
		*/
		void setFreqOffset(RealType offset) noexcept;

		/**
		* @brief Sets the phase offset.
		*
		* @param offset The phase offset to be set.
		*/
		void setPhaseOffset(RealType offset) noexcept;

		/**
		* @brief Sets a random frequency offset.
		*
		* @param stdev The standard deviation for generating the random frequency offset.
		*/
		void setRandomFreqOffset(RealType stdev) noexcept;

		/**
		* @brief Sets a random phase offset.
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
		* @param offsetType The type of offset ("frequency" or "phase") that caused the conflict.
		*/
		void logOffsetConflict(const std::string& offsetType) const noexcept;
	};
}
