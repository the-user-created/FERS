// rspath.h
// Structures and Classes for Paths and Coordinates
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 21 April 2006

// TODO: This needs to be split up into separate files

#ifndef RS_PATH_H
#define RS_PATH_H

#include <stdexcept>
#include <vector>

#include "config.h"
#include "geometry_ops.h"

namespace rs_python
{
	class PythonPath;
}

namespace rs
{
	class MultipathSurface;

	struct Coord
	{
		Vec3 pos;
		RS_FLOAT t;

		bool operator<(const Coord& b) const
		{
			return t < b.t;
		}

		Coord& operator=(const RS_FLOAT a)
		{
			t = a;
			pos.x = pos.y = pos.z = a;
			return *this;
		}
	};

	Coord operator*(const Coord& a, const Coord& b);

	Coord operator+(const Coord& a, const Coord& b);

	Coord operator-(const Coord& a, const Coord& b);

	Coord operator/(const Coord& a, const Coord& b);

	Coord operator+(const Coord& a, RS_FLOAT b);

	Coord operator*(const Coord& a, RS_FLOAT b);

	Coord operator/(RS_FLOAT a, const Coord& b);

	Coord operator/(const Coord& b, RS_FLOAT a);

	struct RotationCoord
	{
		RS_FLOAT azimuth;
		RS_FLOAT elevation;
		RS_FLOAT t;

		bool operator<(const RotationCoord b) const
		{
			return t < b.t;
		}

		RotationCoord& operator=(const RS_FLOAT a)
		{
			azimuth = elevation = t = a;
			return *this;
		}

		explicit RotationCoord(const RS_FLOAT a = 0) : azimuth(a), elevation(a), t(a)
		{
		}
	};

	RotationCoord operator*(const RotationCoord& a, const RotationCoord& b);

	RotationCoord operator+(const RotationCoord& a, const RotationCoord& b);

	RotationCoord operator-(const RotationCoord& a, const RotationCoord& b);

	RotationCoord operator/(const RotationCoord& a, const RotationCoord& b);

	RotationCoord operator+(const RotationCoord& a, RS_FLOAT b);

	RotationCoord operator*(const RotationCoord& a, RS_FLOAT b);

	RotationCoord operator/(RS_FLOAT a, const RotationCoord& b);

	RotationCoord operator/(const RotationCoord& b, RS_FLOAT a);

	class Path
	{
	public:
		enum InterpType { RS_INTERP_STATIC, RS_INTERP_LINEAR, RS_INTERP_CUBIC, RS_INTERP_PYTHON };

		explicit Path(InterpType type = RS_INTERP_STATIC);

		void addCoord(const Coord& coord);

		void finalize();

		[[nodiscard]] Vec3 getPosition(RS_FLOAT t) const;

		void setInterp(InterpType settype);

		void loadPythonPath(const std::string& modname, const std::string& pathname);

	private:
		std::vector<Coord> _coords;
		std::vector<Coord> _dd;
		bool _final;
		InterpType _type;
		rs_python::PythonPath* _pythonpath;

		friend Path* reflectPath(const Path* path, const MultipathSurface* surf);
	};

	Path* reflectPath(const Path* path, const MultipathSurface* surf);

	class RotationPath
	{
	public:
		enum InterpType { RS_INTERP_STATIC, RS_INTERP_CONSTANT, RS_INTERP_LINEAR, RS_INTERP_CUBIC };

		explicit RotationPath(InterpType type = RS_INTERP_STATIC);

		void addCoord(const RotationCoord& coord);

		void finalize();

		[[nodiscard]] SVec3 getPosition(RS_FLOAT t) const;

		void setInterp(InterpType setinterp);

		void setConstantRate(const RotationCoord& setstart, const RotationCoord& setrate);

	protected:
		std::vector<RotationCoord> _coords;
		std::vector<RotationCoord> _dd;
		bool _final;
		RotationCoord _start;
		RotationCoord _rate;
		InterpType _type;

		friend RotationPath* reflectPath(const RotationPath* path, const MultipathSurface* surf);
	};

	RotationPath* reflectPath(const RotationPath* path, const MultipathSurface* surf);

	class PathException final : public std::runtime_error
	{
	public:
		explicit PathException(const std::string& description)
			: std::runtime_error("Error While Executing Path Code: " + description)
		{
		}
	};
}

#endif
