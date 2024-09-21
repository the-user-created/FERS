// platform.h
// Simulator Platform Object
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 21 April 2006

#ifndef PLATFORM_H
#define PLATFORM_H

#include <optional>

#include "math_utils/path.h"
#include "math_utils/rotation_path.h"

namespace radar
{
	class Platform
	{
	public:
		explicit Platform(std::string name) : _motion_path(std::make_unique<math::Path>()),
		_rotation_path(std::make_unique<math::RotationPath>()), _name(std::move(name)), _dual(nullptr) {}

		~Platform() = default;

		// Delete copy constructor and assignment operator to prevent copying
		Platform(const Platform&) = delete;
		Platform& operator=(const Platform&) = delete;

		[[nodiscard]] math::Path* getMotionPath() const { return _motion_path.get(); }

		void setMotionPath(std::unique_ptr<math::Path> path) { _motion_path = std::move(path); }

		[[nodiscard]] math::RotationPath* getRotationPath() const { return _rotation_path.get(); }

		void setRotationPath(std::unique_ptr<math::RotationPath> path) { _rotation_path = std::move(path); }

		[[nodiscard]] math::Vec3 getPosition(const RS_FLOAT time) const { return _motion_path->getPosition(time); }

		[[nodiscard]] math::SVec3 getRotation(const RS_FLOAT time) const { return _rotation_path->getPosition(time); }

		[[nodiscard]] std::string getName() const { return _name; }

		[[nodiscard]] std::optional<Platform*> getDual() const
		{
			if (!_dual)
			{
				return std::nullopt;
			}
			return _dual;
		}

		void setDual(Platform* dual) { _dual = dual; }

	private:
		std::unique_ptr<math::Path> _motion_path;
		std::unique_ptr<math::RotationPath> _rotation_path;
		std::string _name;
		Platform* _dual;
	};

	Platform* createMultipathDual(const Platform* plat, const math::MultipathSurface* surf);
}

#endif
