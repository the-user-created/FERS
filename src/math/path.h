// path.h
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#pragma once

#include <memory>                     // for unique_ptr
#include <string_view>                // for string_view
#include <vector>                     // for vector

#include "config.h"                   // for RealType
#include "coord.h"                    // for Coord
#include "python/python_extension.h"  // for PythonPath

namespace math
{
	class MultipathSurface;
	class Vec3;

	class Path
	{
	public:
		enum class InterpType { INTERP_STATIC, INTERP_LINEAR, INTERP_CUBIC, INTERP_PYTHON };

		explicit Path(const InterpType type = InterpType::INTERP_STATIC) noexcept : _type(type) {}

		~Path() = default;

		void addCoord(const Coord& coord);

		void finalize();

		[[nodiscard]] python::PythonPath* getPythonPath() const noexcept { return _pythonpath.get(); }
		[[nodiscard]] InterpType getType() const noexcept { return _type; }
		[[nodiscard]] const std::vector<Coord>& getCoords() const noexcept { return _coords; }

		[[nodiscard]] Vec3 getPosition(RealType t) const;

		void setPythonPath(std::string_view modname, std::string_view pathname);

		void setInterp(InterpType settype) noexcept;

	private:
		std::vector<Coord> _coords;
		std::vector<Coord> _dd;
		bool _final{false};
		InterpType _type;
		std::unique_ptr<python::PythonPath> _pythonpath{nullptr};
	};

	std::unique_ptr<Path> reflectPath(const Path* path, const MultipathSurface* surf);
}
