/**
* @file timing.h
* @brief Timing source for simulation objects.

* All objects must adhere to a common timing source, which is modeled and adjusted by the methods in this class.
*
* @authors David Young, Marc Brooker
* @date 2006-10-16
*/

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "config.h"
#include "noise/noise_generators.h"

namespace timing
{
	class PrototypeTiming;

	/**
	* @class Timing
	* @brief Represents a timing source for simulation.
	*/
	class Timing final
	{
	public:
		/**
		* @brief Constructs a Timing object.
		*
		* @param name The name of the timing source.
		*/
		explicit Timing(std::string name) noexcept : _name(std::move(name)) {}

		~Timing() = default;
		Timing(const Timing&) = delete;
		Timing& operator=(const Timing&) = delete;
		Timing(Timing&&) = delete;
		Timing& operator=(Timing&&) = delete;

		/**
		* @brief Gets the next sample from the timing source.
		*
		* @return The next sample value or 0.0f if not enabled.
		*/
		[[nodiscard]] RealType getNextSample() const noexcept { return _enabled ? _model->getSample() : 0.0f; }

		/**
		* @brief Gets the name of the timing source.
		*
		* @return The name of the timing source.
		*/
		[[nodiscard]] std::string getName() const noexcept { return _name; }

		/**
		* @brief Checks if the timing source synchronizes on pulse.
		*
		* @return True if synchronized on pulse, otherwise false.
		*/
		[[nodiscard]] bool getSyncOnPulse() const noexcept { return _sync_on_pulse; }

		/**
		* @brief Gets the frequency of the timing source.
		*
		* @return The frequency of the timing source.
		*/
		[[nodiscard]] RealType getFrequency() const noexcept { return _frequency; }

		/**
		* @brief Checks if the timing source is enabled.
		*
		* @return True if enabled, otherwise false.
		*/
		[[nodiscard]] bool isEnabled() const noexcept { return _enabled && _model && _model->enabled(); }

		// Note: This function is not used in the codebase
		/*[[nodiscard]] RealType getPulseTimeError() const noexcept {
			return _enabled && _model ? _model->getSample() : 0.0f;
		}*/

		/**
		* @brief Skips a number of samples in the timing model.
		*
		* @param samples The number of samples to skip.
		*/
		void skipSamples(long long samples) const noexcept;

		/**
		* @brief Initializes the timing model.
		*
		* @param timing The prototype timing configuration used for initialization.
		*/
		void initializeModel(const PrototypeTiming* timing) noexcept;

		/**
		* @brief Resets the timing model.
		*/
		void reset() const noexcept { if (_model) { _model->reset(); } }

	private:
		std::string _name; ///< The name of the timing source.
		bool _enabled{false}; ///< Flag indicating if the timing source is enabled.
		std::unique_ptr<noise::ClockModelGenerator> _model{nullptr};
		///< The noise generator model for the timing source.
		std::vector<RealType> _alphas; ///< The alpha values for the noise generator model.
		std::vector<RealType> _weights; ///< The weights for the noise generator model.
		RealType _frequency{}; ///< The frequency of the timing source.
		bool _sync_on_pulse{false}; ///< Flag indicating if the timing source synchronizes on pulse.
	};
}
