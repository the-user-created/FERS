// path.h
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef PATH_H
#define PATH_H

#include <memory>
#include <string>
#include <vector>

#include "config.h"
#include "coord.h"
#include "python/python_extension.h"

namespace rs_python
{
	class PythonPath;
}

namespace rs
{
	class MultipathSurface;
}

namespace path
{
	class Path
	{
	public:
		enum class InterpType { INTERP_STATIC, INTERP_LINEAR, INTERP_CUBIC, INTERP_PYTHON };

		explicit Path(const InterpType type = InterpType::INTERP_STATIC) : _final(false), _type(type), _pythonpath(nullptr) {}

		~Path() = default;

		void addCoord(const coord::Coord& coord);

		void finalize();

		[[nodiscard]] rs::Vec3 getPosition(RS_FLOAT t) const;

		void setInterp(InterpType settype);

		void loadPythonPath(const std::string& modname, const std::string& pathname);

		[[nodiscard]] rs_python::PythonPath* getPythonPath() const { return _pythonpath.get(); }

		[[nodiscard]] InterpType getType() const { return _type; }

		[[nodiscard]] std::vector<coord::Coord> getCoords() const { return _coords; }

	private:
		std::vector<coord::Coord> _coords;
		std::vector<coord::Coord> _dd;
		bool _final;
		InterpType _type;
		std::unique_ptr<rs_python::PythonPath> _pythonpath;
	};

	std::unique_ptr<Path> reflectPath(const Path* path, const rs::MultipathSurface* surf);
}

#endif //PATH_H
