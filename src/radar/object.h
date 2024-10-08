/**
* @file object.h
* @brief Base class for all physical objects in the radar system.
*
* This file defines the Object class, which represents a physical object within a radar simulation environment.
* The Object class provides interfaces to retrieve positional and rotational data from the platform on which
* the object is situated.
* It also enforces move semantics and disables copy semantics for instances of this class.
*
* @authors David Young, Marc Brooker
* @date 2006-07-19
*/

#pragma once

#include "platform.h"

namespace radar
{
	/**
	* @class Object
	* @brief Represents a physical object in the radar system.
	*
	* The Object class provides a mechanism to access the position and rotation of physical objects in a radar simulation.
	* It interacts with a Platform object to retrieve its positional and rotational data at a given point in time.
	* This class is non-copyable but supports move semantics.
	*/
	class Object
	{
	public:
		/**
		* @brief Constructor for Object.
		*
		* Initializes an Object with the provided platform and name.
		*
		* @param platform Pointer to the Platform object associated with this Object.
		* @param name The name of the Object.
		*/
		Object(Platform* platform, std::string name) noexcept : _platform(platform), _name(std::move(name)) {}

		/**
		* @brief Default virtual destructor.
		*
		* Cleans up the Object instance. Destructor is virtual to ensure proper cleanup of derived objects.
		*/
		virtual ~Object() = default;

		/**
		* @brief Deleted copy constructor.
		*
		* Prevents copying of Object instances.
		*/
		Object(const Object&) = delete;

		/**
		* @brief Deleted copy assignment operator.
		*
		* Prevents assignment of one Object instance to another.
		*/
		Object& operator=(const Object&) = delete;

		/**
		* @brief Default move constructor.
		*
		* Enables moving of Object instances.
		*/
		Object(Object&&) noexcept = default;

		/**
		* @brief Default move assignment operator.
		*
		* Enables move assignment for Object instances.
		*/
		Object& operator=(Object&&) noexcept = default;

		/**
		* @brief Retrieves the position of the object.
		*
		* Obtains the position of the object from its associated platform at a specified time.
		*
		* @param time The time at which to get the position of the object.
		* @return A math::Vec3 representing the position of the object.
		*/
		[[nodiscard]] math::Vec3 getPosition(const RealType time) const { return _platform->getPosition(time); }

		/**
		* @brief Retrieves the rotation of the object.
		*
		* Obtains the rotation of the object from its associated platform at a specified time.
		*
		* @param time The time at which to get the rotation of the object.
		* @return A math::SVec3 representing the rotation of the object.
		*/
		[[nodiscard]] math::SVec3 getRotation(const RealType time) const { return _platform->getRotation(time); }

		/**
		* @brief Retrieves the associated platform of the object.
		*
		* Provides access to the platform on which the object is situated.
		*
		* @return A pointer to the Platform object associated with this object.
		*/
		[[nodiscard]] Platform* getPlatform() const noexcept { return _platform; }

		/**
		* @brief Retrieves the name of the object.
		*
		* Provides the name assigned to the object.
		*
		* @return A const reference to the string representing the object's name.
		*/
		[[nodiscard]] const std::string& getName() const noexcept { return _name; }

	private:
		Platform* _platform; ///< Pointer to the Platform object associated with this Object.
		std::string _name; ///< The name of the Object.
	};
}
