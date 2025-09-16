/**
* @file platform.h
* @brief Defines the Platform class used in radar simulation.
*
* @authors David Young, Marc Brooker
* @date 2006-04-21
*/

#pragma once

#include <memory>
#include <string>
#include <utility>

#include "config.h"
#include "math/geometry_ops.h"
#include "math/path.h"
#include "math/rotation_path.h"

namespace radar
{
	/**
	* @class Platform
	* @brief Represents a simulation platform with motion and rotation paths.
	*/
	class Platform
	{
	public:
		/**
		* @brief Constructs a Platform with the specified name.
		*
		* @param name The name of the platform.
		*/
		explicit Platform(std::string name) noexcept :
			_motion_path(std::make_unique<math::Path>()),
			_rotation_path(std::make_unique<math::RotationPath>()),
			_name(std::move(name)) {}

		Platform(const Platform&) = delete;

		Platform& operator=(const Platform&) = delete;

		~Platform() = default;

		Platform(Platform&&) = default;

		Platform& operator=(Platform&&) = default;

		/**
		* @brief Gets the motion path of the platform.
		*
		* @return A pointer to the motion path of the platform.
		*/
		[[nodiscard]] math::Path* getMotionPath() const noexcept { return _motion_path.get(); }

		/**
		* @brief Gets the rotation path of the platform.
		*
		* @return A pointer to the rotation path of the platform.
		*/
		[[nodiscard]] math::RotationPath* getRotationPath() const noexcept { return _rotation_path.get(); }

		/**
		* @brief Gets the position of the platform at a specific time.
		*
		* @param time The time for which the position is requested.
		* @return A vector representing the position of the platform.
		*/
		[[nodiscard]] math::Vec3 getPosition(const RealType time) const { return _motion_path->getPosition(time); }

		/**
		* @brief Gets the rotation of the platform at a specific time.
		*
		* @param time The time for which the rotation is requested.
		* @return A vector representing the rotation of the platform.
		*/
		[[nodiscard]] math::SVec3 getRotation(const RealType time) const { return _rotation_path->getPosition(time); }

		/**
		* @brief Gets the name of the platform.
		*
		* @return A constant reference to the name of the platform.
		*/
		[[nodiscard]] const std::string& getName() const noexcept { return _name; }

		/**
		* @brief Sets the rotation path of the platform.
		*
		* @param path A unique pointer to the new rotation path.
		*/
		void setRotationPath(std::unique_ptr<math::RotationPath> path) noexcept { _rotation_path = std::move(path); }

		/**
		* @brief Sets the motion path of the platform.
		*
		* @param path A unique pointer to the new motion path.
		*/
		void setMotionPath(std::unique_ptr<math::Path> path) noexcept { _motion_path = std::move(path); }

	private:
		std::unique_ptr<math::Path> _motion_path; ///< The motion path of the platform.
		std::unique_ptr<math::RotationPath> _rotation_path; ///< The rotation path of the platform.
		std::string _name; ///< The name of the platform.
	};
}
