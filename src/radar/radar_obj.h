/**
* @file radar_obj.h
* @brief Defines the Radar class and associated functionality.
*
* This file contains the declaration of the `Radar` class, which models radar objects.
* It includes functions to manage radar-specific properties such as antenna configuration,
* timing, multipath effects, and noise temperature.
* It also provides template functionality
* to create multipath dual objects for radar components like transmitters and receivers.
*
* @authors David Young, Marc Brooker
* @date 2006-04-21
*/

#pragma once

#include "object.h"

namespace timing
{
	class Timing;
}

namespace antenna
{
	class Antenna;
}

namespace radar
{
	class Platform;

	/**
	* @class Radar
	* @brief Represents a radar system on a platform.
	*
	* The `Radar` class provides functionality for managing radar-related properties, including antenna configuration,
	* timing sources, and multipath reflection handling. It inherits from the `Object` class and offers a set of
	* radar-specific behaviors such as calculating gain, handling noise temperature, and managing multipath effects.
	*/
	class Radar : public Object
	{
	public:
		/**
		* @brief Constructs a Radar object.
		*
		* Creates a new `Radar` object attached to a given platform with a specific name.
		*
		* @param platform Pointer to the platform on which the radar is mounted.
		* @param name Name of the radar object.
		*/
		Radar(Platform* platform, std::string name) noexcept : Object(platform, std::move(name)) {}

		/**
		* @brief Destructor for Radar class.
		*
		* Cleans up any resources used by the `Radar` object.
		*/
		~Radar() override = default;

		/**
		* @brief Checks if multipath dual mode is enabled.
		*
		* Returns whether multipath dual is enabled for this radar.
		*
		* @return True if multipath dual is enabled, false otherwise.
		*/
		[[nodiscard]] bool getMultipathDual() const noexcept { return _multipath_dual; }

		/**
		* @brief Retrieves the attached radar object.
		*
		* Returns a pointer to the radar object that is attached to this radar.
		*
		* @return Pointer to the attached radar object.
		*/
		[[nodiscard]] const Radar* getAttached() const noexcept { return _attached; }

		/**
		* @brief Gets the multipath reflection factor.
		*
		* Returns the factor used for multipath reflection calculations.
		*
		* @return Multipath reflection factor.
		*/
		[[nodiscard]] RealType getMultipathFactor() const noexcept { return _multipath_factor; }

		/**
		* @brief Gets the antenna associated with this radar.
		*
		* Returns a pointer to the antenna object used by this radar.
		*
		* @return Pointer to the associated antenna.
		*/
		[[nodiscard]] const antenna::Antenna* getAntenna() const noexcept { return _antenna; }

		/**
		* @brief Calculates the radar gain based on input angles and wavelength.
		*
		* This function computes the radar gain for a specific angle and reference angle,
		* based on the wavelength of the radar signal.
		*
		* @param angle The radar's pointing angle.
		* @param refangle The reference angle for comparison.
		* @param wavelength The wavelength of the radar signal.
		* @return The calculated radar gain.
		*/
		[[nodiscard]] RealType getGain(const math::SVec3& angle, const math::SVec3& refangle,
		                               RealType wavelength) const;

		/**
		* @brief Gets the noise temperature of the radar.
		*
		* Calculates the radar's noise temperature at a specific angle.
		*
		* @param angle The angle at which the noise temperature is calculated.
		* @return The calculated noise temperature.
		*/
		[[nodiscard]] virtual RealType getNoiseTemperature(const math::SVec3& angle) const noexcept;

		/**
		* @brief Retrieves the timing source for the radar.
		*
		* Returns the timing source associated with this radar.
		*
		* @return Shared pointer to the timing source.
		*/
		[[nodiscard]] std::shared_ptr<timing::Timing> getTiming() const;

		/**
		* @brief Sets the timing source for the radar.
		*
		* Associates a timing source with this radar object.
		*
		* @param tim Shared pointer to the timing source to set.
		*/
		void setTiming(const std::shared_ptr<timing::Timing>& tim);

		/**
		* @brief Sets the antenna for the radar.
		*
		* Assigns an antenna object to the radar.
		*
		* @param ant Pointer to the antenna to set.
		*/
		void setAntenna(const antenna::Antenna* ant);

		/**
		* @brief Enables multipath dual mode.
		*
		* Enables multipath dual mode and sets the reflection factor.
		*
		* @param reflect The reflection factor to be set for multipath dual mode.
		*/
		void setMultipathDual(RealType reflect) noexcept;

		/**
		* @brief Attaches another radar object to this radar.
		*
		* Links another radar object to this radar.
		* Throws an exception if another object is already attached.
		*
		* @param obj Pointer to the radar object to attach.
		* @throws std::runtime_error If another object is already attached.
		*/
		void setAttached(const Radar* obj);

	protected:
		std::shared_ptr<timing::Timing> _timing; ///< Timing source for the radar.

	private:
		const antenna::Antenna* _antenna{nullptr}; ///< Antenna object associated with the radar.
		const Radar* _attached{nullptr}; ///< Attached radar object.
		bool _multipath_dual{false}; ///< Flag indicating if multipath dual mode is enabled.
		RealType _multipath_factor{0}; ///< Multipath reflection factor.
	};

	/**
	* @brief Creates a multipath dual base object.
	*
	* This function template creates a dual object for radar components (e.g., transmitters or receivers)
	* based on multipath surface characteristics.
	*
	* @tparam T The type of the object for which the dual is created (e.g., Transmitter, Receiver).
	* @param obj Pointer to the original object.
	* @param surf Pointer to the multipath surface defining the reflection characteristics.
	* @return Pointer to the newly created dual object.
	*/
	template <typename T>
	T* createMultipathDualBase(T* obj, const math::MultipathSurface* surf);
}
