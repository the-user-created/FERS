// path.h
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef PATH_H
#define PATH_H

#include <string>
#include <vector>

#include "config.h"
#include "coord.h"

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
		enum InterpType { RS_INTERP_STATIC, RS_INTERP_LINEAR, RS_INTERP_CUBIC, RS_INTERP_PYTHON };

		explicit Path(InterpType type = RS_INTERP_STATIC);

		void addCoord(const coord::Coord& coord);

		void finalize();

		[[nodiscard]] rs::Vec3 getPosition(RS_FLOAT t) const;

		void setInterp(InterpType settype);

		void loadPythonPath(const std::string& modname, const std::string& pathname);

		[[nodiscard]] rs_python::PythonPath* getPythonPath() const { return _pythonpath; }

		[[nodiscard]] InterpType getType() const { return _type; }

		[[nodiscard]] std::vector<coord::Coord> getCoords() const { return _coords; }

	private:
		std::vector<coord::Coord> _coords;
		std::vector<coord::Coord> _dd;
		bool _final;
		InterpType _type;
		rs_python::PythonPath* _pythonpath;
	};

	Path* reflectPath(const Path* path, const rs::MultipathSurface* surf);
}

#endif //PATH_H
