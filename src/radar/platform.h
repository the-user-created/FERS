/**
* @file platform.h
* @brief Defines the Platform class used in radar simulation.
*
* This file contains the declaration of the Platform class, which represents a platform in a radar simulation.
* The Platform class handles motion and rotation paths
* and can have a dual platform for simulating complex interactions with surfaces.
* It also includes the function declaration for creating a dual platform based on multipath reflections.
*
* @authors David Young, Marc Brooker
* @date 2006-04-21
*/

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "config.h"
#include "math/geometry_ops.h"
#include "math/path.h"
#include "math/rotation_path.h"

namespace math
{
	class MultipathSurface;
}

namespace radar
{
	/**
	* @class Platform
	* @brief Represents a simulation platform with motion and rotation paths.
	*
	* The Platform class manages the motion and rotation paths of a platform
	* in the radar simulation. It supports dual platforms for multipath
	* reflection modeling and provides methods to retrieve the position,
	* rotation, and name of the platform at any given time.
	*/
	class Platform
	{
	public:
		/**
		* @brief Constructs a Platform with the specified name.
		*
		* Initializes a Platform with default motion and rotation paths.
		*
		* @param name The name of the platform.
		*/
		explicit Platform(std::string name) noexcept : _motion_path(std::make_unique<math::Path>()),
		                                               _rotation_path(std::make_unique<math::RotationPath>()),
		                                               _name(std::move(name)) {}

		/**
		* @brief Deleted copy constructor to prevent copying of Platform.
		*/
		Platform(const Platform&) = delete;

		/**
		* @brief Deleted copy assignment operator to prevent copying of Platform.
		*/
		Platform& operator=(const Platform&) = delete;

		/**
		* @brief Defaulted destructor.
		*/
		~Platform() = default;

		/**
		* @brief Gets the motion path of the platform.
		*
		* Retrieves the motion path of the platform, which describes its position over time.
		*
		* @return A pointer to the motion path of the platform.
		*/
		[[nodiscard]] math::Path* getMotionPath() const noexcept { return _motion_path.get(); }

		/**
		* @brief Gets the rotation path of the platform.
		*
		* Retrieves the rotation path of the platform, which describes its rotation over time.
		*
		* @return A pointer to the rotation path of the platform.
		*/
		[[nodiscard]] math::RotationPath* getRotationPath() const noexcept { return _rotation_path.get(); }

		/**
		* @brief Gets the position of the platform at a specific time.
		*
		* Retrieves the 3D position of the platform at the specified time using the motion path.
		*
		* @param time The time for which the position is requested.
		* @return A vector representing the position of the platform.
		*/
		[[nodiscard]] math::Vec3 getPosition(const RealType time) const { return _motion_path->getPosition(time); }

		/**
		* @brief Gets the rotation of the platform at a specific time.
		*
		* Retrieves the rotation of the platform at the specified time using the rotation path.
		*
		* @param time The time for which the rotation is requested.
		* @return A vector representing the rotation of the platform.
		*/
		[[nodiscard]] math::SVec3 getRotation(const RealType time) const { return _rotation_path->getPosition(time); }

		/**
		* @brief Gets the name of the platform.
		*
		* Retrieves the name of the platform as a constant string reference.
		*
		* @return A constant reference to the name of the platform.
		*/
		[[nodiscard]] const std::string& getName() const noexcept { return _name; }

		/**
		* @brief Gets the dual platform, if it exists.
		*
		* Retrieves the dual platform if it has been created, otherwise returns nullopt.
		*
		* @return An optional containing a pointer to the dual platform, or nullopt if none exists.
		*/
		[[nodiscard]] std::optional<Platform*> getDual() const noexcept { return _dual ? std::optional{_dual} : std::nullopt; }

		/**
		* @brief Sets the rotation path of the platform.
		*
		* Replaces the current rotation path with a new one.
		*
		* @param path A unique pointer to the new rotation path.
		*/
		void setRotationPath(std::unique_ptr<math::RotationPath> path) noexcept { _rotation_path = std::move(path); }

		/**
		* @brief Sets the motion path of the platform.
		*
		* Replaces the current motion path with a new one.
		*
		* @param path A unique pointer to the new motion path.
		*/
		void setMotionPath(std::unique_ptr<math::Path> path) noexcept { _motion_path = std::move(path); }

		/**
		* @brief Sets the dual platform.
		*
		* Assigns a dual platform to this platform, typically used for multipath simulations.
		*
		* @param dual A pointer to the dual platform.
		*/
		void setDual(Platform* dual) noexcept { _dual = dual; }

	private:
		std::unique_ptr<math::Path> _motion_path; ///< The motion path of the platform.
		std::unique_ptr<math::RotationPath> _rotation_path; ///< The rotation path of the platform.
		std::string _name; ///< The name of the platform.
		Platform* _dual = nullptr; ///< The dual platform, if it exists.
	};

	/**
	* @brief Creates a dual platform based on multipath reflections.
	*
	* Generates a dual platform for the given platform by reflecting its motion and
	* rotation paths across a multipath surface.
	*
	* @param plat The original platform for which the dual is created.
	* @param surf The multipath surface used to reflect the paths.
	* @return A pointer to the newly created dual platform.
	*/
	Platform* createMultipathDual(Platform* plat, const math::MultipathSurface* surf);
}
