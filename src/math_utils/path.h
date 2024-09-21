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
#include "python/python_extension.h"

namespace math
{
	class MultipathSurface;
	struct Coord;
	struct Vec3;

	class Path
	{
	public:
		enum class InterpType { INTERP_STATIC, INTERP_LINEAR, INTERP_CUBIC, INTERP_PYTHON };

		explicit Path(const InterpType type = InterpType::INTERP_STATIC) : _final(false), _type(type),
		                                                                   _pythonpath(nullptr) {}

		~Path() = default;

		void addCoord(const Coord& coord);

		void finalize();

		[[nodiscard]] Vec3 getPosition(RealType t) const;

		void setInterp(InterpType settype);

		void loadPythonPath(const std::string& modname, const std::string& pathname);

		[[nodiscard]] python::PythonPath* getPythonPath() const { return _pythonpath.get(); }

		[[nodiscard]] InterpType getType() const { return _type; }

		[[nodiscard]] std::vector<Coord> getCoords() const { return _coords; }

	private:
		std::vector<Coord> _coords;
		std::vector<Coord> _dd;
		bool _final;
		InterpType _type;
		std::unique_ptr<python::PythonPath> _pythonpath;
	};

	std::unique_ptr<Path> reflectPath(const Path* path, const MultipathSurface* surf);
}

#endif //PATH_H
