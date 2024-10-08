/**
 * @file receiver.h
 * @brief Radar Receiver class for managing signal reception and response handling.
 *
 * This header file defines the `Receiver` class, which inherits from the `Radar` class
 * and provides functionalities for managing the reception of radar signals.
 * It handles the configuration of radar windows, responses, noise temperature, and dual receivers.
 * It also manages flags for processing radar signals.
 *
 * @authors David Young, Marc Brooker
 * @date 2024-10-07
 */

#pragma once

#include "radar_obj.h"
#include "serial/response.h"

namespace radar
{
	/**
	 * @class Receiver
	 * @brief Manages radar signal reception and response processing.
	 *
	 * The `Receiver` class extends the `Radar` class to provide additional features
	 * related to signal reception, such as noise temperature management, window properties, and response collection.
	 * It supports multiple configuration flags and the ability to work in dual receiver mode.
	 */
	class Receiver final : public Radar
	{
	public:
		/**
		 * @enum RecvFlag
		 * @brief Enumeration for receiver configuration flags.
		 *
		 * Defines the available flags for controlling how the receiver processes
		 * incoming radar signals.
		 */
		enum class RecvFlag { FLAG_NODIRECT = 1, FLAG_NOPROPLOSS = 2 };

		/**
		 * @brief Constructs a Receiver object.
		 *
		 * Initializes a Receiver with a given platform and name.
		 *
		 * @param platform The platform associated with this receiver.
		 * @param name The name of the receiver. Defaults to "defRecv".
		 */
		explicit Receiver(Platform* platform,
		                  std::string name = "defRecv") noexcept : Radar(platform, std::move(name)) {}

		/**
		 * @brief Destructor for Receiver.
		 *
		 * Cleans up resources used by the Receiver object.
		 */
		~Receiver() override = default;

		/**
		 * @brief Adds a response to the receiver.
		 *
		 * Adds a unique response object to the list of responses the receiver handles.
		 *
		 * @param response A unique pointer to the response object.
		 */
		void addResponse(std::unique_ptr<serial::Response> response) noexcept;

		/**
		 * @brief Checks if a specific flag is set.
		 *
		 * Determines if the specified flag is currently set in the receiver.
		 *
		 * @param flag The flag to check.
		 * @return True if the flag is set, false otherwise.
		 */
		[[nodiscard]] bool checkFlag(RecvFlag flag) const noexcept { return _flags & static_cast<int>(flag); }

		/**
		 * @brief Renders the responses and exports the data.
		 *
		 * Processes the stored responses, sorts them, and exports the data
		 * based on user preferences in XML, binary, or CSV format.
		 */
		void render();

		/**
		 * @brief Retrieves the noise temperature of the receiver.
		 *
		 * Gets the current noise temperature of the receiver.
		 *
		 * @return The noise temperature.
		 */
		[[nodiscard]] RealType getNoiseTemperature() const noexcept { return _noise_temperature; }

		/**
		 * @brief Retrieves the radar window length.
		 *
		 * Gets the configured length of the radar window.
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
		 * Gets the configured window skip time for the radar.
		 *
		 * @return The window skip time.
		 */
		[[nodiscard]] RealType getWindowSkip() const noexcept { return _window_skip; }

		/**
		 * @brief Gets the dual receiver.
		 *
		 * Retrieves the pointer to the dual receiver if one is configured.
		 *
		 * @return Pointer to the dual receiver.
		 */
		[[nodiscard]] Receiver* getDual() const noexcept { return _dual; }

		/**
		 * @brief Gets the noise temperature for a specific angle.
		 *
		 * Calculates the noise temperature at a given angle.
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
		[[nodiscard]] RealType getWindowStart(int window) const;

		/**
		 * @brief Gets the number of radar windows.
		 *
		 * Calculates the number of windows based on radar parameters.
		 *
		 * @return The total number of radar windows.
		 */
		[[nodiscard]] int getWindowCount() const noexcept;

		/**
		 * @brief Gets the number of responses collected by the receiver.
		 *
		 * @return The number of collected responses.
		 */
		[[nodiscard]] int getResponseCount() const noexcept;

		/**
		 * @brief Sets the properties for radar windows.
		 *
		 * Configures the radar window's length, PRF, and skip time.
		 *
		 * @param length The length of the radar window.
		 * @param prf The pulse repetition frequency.
		 * @param skip The skip time between windows.
		 */
		void setWindowProperties(RealType length, RealType prf, RealType skip) noexcept;

		/**
		 * @brief Sets a receiver flag.
		 *
		 * Configures the receiver with the specified flag.
		 *
		 * @param flag The flag to set.
		 */
		void setFlag(RecvFlag flag) noexcept { _flags |= static_cast<int>(flag); }

		/**
		 * @brief Sets the dual receiver.
		 *
		 * Configures a dual receiver for this receiver.
		 *
		 * @param dual The dual receiver.
		 */
		void setDual(Receiver* dual) noexcept { _dual = dual; }

		/**
		 * @brief Sets the noise temperature of the receiver.
		 *
		 * Configures the noise temperature of the receiver.
		 * Throws an exception if the temperature is negative.
		 *
		 * @param temp The new noise temperature.
		 * @throws std::runtime_error If the noise temperature is negative.
		 */
		void setNoiseTemperature(RealType temp);

	private:
		std::vector<std::unique_ptr<serial::Response>> _responses; ///< The list of responses.
		mutable std::mutex _responses_mutex; ///< Mutex for handling responses.
		RealType _noise_temperature = 0; ///< The noise temperature of the receiver.
		RealType _window_length = 0; ///< The length of the radar window.
		RealType _window_prf = 0; ///< The pulse repetition frequency (PRF) of the radar window.
		RealType _window_skip = 0; ///< The skip time between radar windows.
		Receiver* _dual = nullptr; ///< The dual receiver.
		int _flags = 0; ///< Flags for receiver configuration.
	};

	/**
	 * @brief Creates a dual receiver.
	 *
	 * Creates a dual receiver based on the given receiver and surface.
	 *
	 * @param recv The receiver to create a dual receiver for.
	 * @param surf The multipath surface to use for the dual receiver.
	 * @return The dual receiver.
	 */
	inline Receiver* createMultipathDual(Receiver* recv, const math::MultipathSurface* surf)
	{
		return createMultipathDualBase(recv, surf);
	}
}
