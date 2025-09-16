/**
 * @file receiver.h
 * @brief Radar Receiver class for managing signal reception and response handling.
 *
 * @authors David Young, Marc Brooker
 * @date 2024-10-07
 */

#pragma once

#include <random>
#include <mutex>

#include "radar_obj.h"
#include "serial/response.h"

namespace pool
{
	class ThreadPool;
}

namespace radar
{
	/**
	 * @class Receiver
	 * @brief Manages radar signal reception and response processing.
	 */
	class Receiver final : public Radar
	{
	public:
		/**
		 * @enum RecvFlag
		 * @brief Enumeration for receiver configuration flags.
		 */
		enum class RecvFlag { FLAG_NODIRECT = 1, FLAG_NOPROPLOSS = 2 };

		/**
		 * @brief Constructs a Receiver object.
		 *
		 * @param platform The platform associated with this receiver.
		 * @param name The name of the receiver. Defaults to "defRecv".
		 * @param seed The seed for the receiver's internal random number generator.
		 */
		explicit Receiver(Platform* platform, std::string name, unsigned seed) noexcept;

		~Receiver() override = default;

		Receiver(const Receiver&) = delete;

		Receiver(Receiver&&) = delete;

		Receiver& operator=(const Receiver&) = delete;

		Receiver& operator=(Receiver&&) = delete;

		/**
		 * @brief Adds a response to the receiver.
		 *
		 * @param response A unique pointer to the response object.
		 */
		void addResponse(std::unique_ptr<serial::Response> response) noexcept;

		/**
		 * @brief Checks if a specific flag is set.
		 *
		 * @param flag The flag to check.
		 * @return True if the flag is set, false otherwise.
		 */
		[[nodiscard]] bool checkFlag(RecvFlag flag) const noexcept { return _flags & static_cast<int>(flag); }

		/**
		 * @brief Renders the responses and exports the data to a file.
		 */
		void render(pool::ThreadPool& pool);

		/**
		 * @brief Retrieves the noise temperature of the receiver.
		 *
		 * @return The noise temperature.
		 */
		[[nodiscard]] RealType getNoiseTemperature() const noexcept { return _noise_temperature; }

		/**
		 * @brief Retrieves the radar window length.
		 *
		 * @return The radar window length.
		 */
		[[nodiscard]] RealType getWindowLength() const noexcept { return _window_length; }

		/**
		 * @brief Retrieves the pulse repetition frequency (PRF) of the radar window.
		 *
		 * @return The PRF of the radar window.
		 */
		[[nodiscard]] RealType getWindowPrf() const noexcept { return _window_prf; }

		/**
		 * @brief Retrieves the window skip time.
		 *
		 * @return The window skip time.
		 */
		[[nodiscard]] RealType getWindowSkip() const noexcept { return _window_skip; }

		/**
		 * @brief Gets the noise temperature for a specific angle.
		 *
		 * @param angle The angle in spherical coordinates (SVec3).
		 * @return The noise temperature at the given angle.
		 */
		[[nodiscard]] RealType getNoiseTemperature(const math::SVec3& angle) const noexcept override;

		/**
		 * @brief Retrieves the start time of a specific radar window.
		 *
		 * @param window The index of the window.
		 * @return The start time of the specified window.
		 * @throws std::logic_error If the receiver is not associated with a timing source.
		 */
		[[nodiscard]] RealType getWindowStart(unsigned window) const;

		/**
		 * @brief Gets the number of radar windows.
		 *
		 * @return The total number of radar windows.
		 */
		[[nodiscard]] unsigned getWindowCount() const noexcept;

		/**
		 * @brief Gets the number of responses collected by the receiver.
		 *
		 * @return The number of collected responses.
		 */
		[[nodiscard]] int getResponseCount() const noexcept;

		/**
		* @brief Gets the receiver's internal random number generator engine.
		* @return A mutable reference to the RNG engine.
		*/
		[[nodiscard]] std::mt19937& getRngEngine() noexcept { return _rng; }

		/**
		 * @brief Sets the properties for radar windows.
		 *
		 * @param length The length of the radar window.
		 * @param prf The pulse repetition frequency.
		 * @param skip The skip time between windows.
		 */
		void setWindowProperties(RealType length, RealType prf, RealType skip) noexcept;

		/**
		 * @brief Sets a receiver flag.
		 *
		 * @param flag The flag to set.
		 */
		void setFlag(RecvFlag flag) noexcept { _flags |= static_cast<int>(flag); }

		/**
		 * @brief Sets the noise temperature of the receiver.
		 *
		 * @param temp The new noise temperature.
		 * @throws std::runtime_error If the noise temperature is negative.
		 */
		void setNoiseTemperature(RealType temp);

		/**
		 * @brief Prepares the internal storage for CW IQ data.
		 * @param numSamples The total number of samples to allocate memory for.
		 */
		void prepareCwData(size_t numSamples);

		/**
		 * @brief Sets a single IQW sample at a specific index for CW simulation.
		 * @param index The index at which to store the sample.
		 * @param sample The complex IQ sample.
		 */
		void setCwSample(size_t index, ComplexType sample);

		/**
		 * @brief Retrieves the collected CW IQ data.
		 * @return A constant reference to the vector of complex IQ samples.
		 */
		[[nodiscard]] const std::vector<ComplexType>& getCwData() const { return _cw_iq_data; }

	private:
		std::vector<std::unique_ptr<serial::Response>> _responses; ///< The list of responses.
		std::mutex _responses_mutex; ///< Mutex for handling responses.
		RealType _noise_temperature = 0; ///< The noise temperature of the receiver.
		RealType _window_length = 0; ///< The length of the radar window.
		RealType _window_prf = 0; ///< The pulse repetition frequency (PRF) of the radar window.
		RealType _window_skip = 0; ///< The skip time between radar windows.
		int _flags = 0; ///< Flags for receiver configuration.
		std::vector<ComplexType> _cw_iq_data; ///< IQ data for CW simulations.
		std::mutex _cw_mutex; ///< Mutex for handling CW data.
		std::mt19937 _rng; ///< Per-object random number generator for statistical independence.
	};
}
