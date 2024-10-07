// platform.h
// Simulator Platform Object
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 21 April 2006

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
	class Platform
	{
	public:
		explicit Platform(std::string name) noexcept : _motion_path(std::make_unique<math::Path>()),
		                                               _rotation_path(std::make_unique<math::RotationPath>()),
		                                               _name(std::move(name)) {}

		// Delete copy constructor and assignment operator to prevent copying
		Platform(const Platform&) = delete;

		Platform& operator=(const Platform&) = delete;

		// Defaulted destructor
		~Platform() = default;

		[[nodiscard]] math::Path* getMotionPath() const noexcept { return _motion_path.get(); }
		[[nodiscard]] math::RotationPath* getRotationPath() const noexcept { return _rotation_path.get(); }
		[[nodiscard]] math::Vec3 getPosition(const RealType time) const { return _motion_path->getPosition(time); }
		[[nodiscard]] math::SVec3 getRotation(const RealType time) const { return _rotation_path->getPosition(time); }
		[[nodiscard]] const std::string& getName() const noexcept { return _name; }
		[[nodiscard]] std::optional<Platform*> getDual() const noexcept { return _dual ? std::optional{_dual} : std::nullopt; }

		void setRotationPath(std::unique_ptr<math::RotationPath> path) noexcept { _rotation_path = std::move(path); }
		void setMotionPath(std::unique_ptr<math::Path> path) noexcept { _motion_path = std::move(path); }
		void setDual(Platform* dual) noexcept { _dual = dual; }

	private:
		std::unique_ptr<math::Path> _motion_path;
		std::unique_ptr<math::RotationPath> _rotation_path;
		std::string _name;
		Platform* _dual = nullptr;
	};

	Platform* createMultipathDual(Platform* plat, const math::MultipathSurface* surf);
}
