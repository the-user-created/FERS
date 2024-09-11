//rspath.h
//Structures and Classes for Paths and Coordinates
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 21 April 2006

#ifndef RS_PATH_H
#define RS_PATH_H

#include <stdexcept>
#include <vector>

#include "config.h"
#include "rsgeometry.h"

//Forward declarations
namespace rs_python
{
	class PythonPath; //rspython.h
}

namespace rs
{
	//Forward declaration of MultipathSurface (rsmultipath.h)
	class MultipathSurface;


	/// Coord holds a three dimensional spatial co-ordinate and a time value
	struct Coord
	{
		Vec3 pos; //!< Position in space
		RS_FLOAT t; //!< Time

		/// Less than comparison operator. Operates only on time value.
		bool operator<(const Coord& b) const
		{
			return t < b.t;
		}

		/// Assignment operator
		Coord& operator=(const RS_FLOAT a)
		{
			t = a;
			pos.x = pos.y = pos.z = a;
			return *this;
		}
	};

	//Support arithmetic on path coords.
	/// Componentwise multiply
	Coord operator*(const Coord& a, const Coord& b);

	/// Componentwise add
	Coord operator+(const Coord& a, const Coord& b);

	/// Componentwise subtract
	Coord operator-(const Coord& a, const Coord& b);

	/// Componentwise divide
	Coord operator/(const Coord& a, const Coord& b);

	/// Add a scalar to each element
	Coord operator+(const Coord& a, RS_FLOAT b);

	/// Componentwise multiplication by scalar
	Coord operator*(const Coord& a, RS_FLOAT b);

	/// Componentwise division by scalar
	Coord operator/(RS_FLOAT a, const Coord& b);

	Coord operator/(const Coord& b, RS_FLOAT a);

	/// Azimuth-Elevation rotation position and time
	//Note that the plane for azimuth is the x-y plane (see Coord)
	struct RotationCoord
	{
		RS_FLOAT azimuth; //!< Angle in X-Y plane (radians)
		RS_FLOAT elevation; //!< Elevation above X-Y plane (radians)
		RS_FLOAT t; //!< Time

		/// Less than comparison operator. Operates only on time value.
		bool operator<(const RotationCoord b) const
		{
			return t < b.t;
		}

		/// Assignment operator
		RotationCoord& operator=(const RS_FLOAT a)
		{
			azimuth = elevation = t = a;
			return *this;
		}

		/// Constructor. Assign scalar to all elements.
		explicit RotationCoord(const RS_FLOAT a = 0):
			azimuth(a), elevation(a), t(a)
		{
		}
	};

	/// Componentwise Multiply
	RotationCoord operator*(const RotationCoord& a, const RotationCoord& b);

	/// Componentwise Add
	RotationCoord operator+(const RotationCoord& a, const RotationCoord& b);

	/// Componentwise Subtract
	RotationCoord operator-(const RotationCoord& a, const RotationCoord& b);

	/// Componentwise Divide
	RotationCoord operator/(const RotationCoord& a, const RotationCoord& b);

	/// Add a scalare to each component
	RotationCoord operator+(const RotationCoord& a, RS_FLOAT b);

	/// Multiply each component by a scalar
	RotationCoord operator*(const RotationCoord& a, RS_FLOAT b);

	/// Divide each component by a scalar
	RotationCoord operator/(RS_FLOAT a, const RotationCoord& b);

	RotationCoord operator/(const RotationCoord& b, RS_FLOAT a);

	/// Defines the movement of an object through space, governed by time
	class Path
	{
	public:
		enum InterpType { RS_INTERP_STATIC, RS_INTERP_LINEAR, RS_INTERP_CUBIC, RS_INTERP_PYTHON };

		/// Default constructor
		explicit Path(InterpType type = RS_INTERP_STATIC);

		/// Add a co-ordinate to the path
		void addCoord(const Coord& coord);

		/// Finalize the path, so it can be requested with getPosition
		void finalize();

		/// Get the position an object will be at at a specified time
		[[nodiscard]] Vec3 getPosition(RS_FLOAT t) const;

		/// Set the interpolation type of the path
		void setInterp(InterpType settype);

		/// Load a python path function
		void loadPythonPath(const std::string& modname, const std::string& pathname);

	private:
		std::vector<Coord> _coords; //!< Vector of time and space coordinates
		std::vector<Coord> _dd; //!< Vector of second derivatives at points (used for cubic interpolation)
		bool _final; //!< Is the path finalised?
		InterpType _type; //!< Type of interpolation
		rs_python::PythonPath* _pythonpath; //!< Pointer to the PythonPath object
		/// Create a new path which is a reflection of this one around the given plane
		friend Path* reflectPath(const Path* path, const MultipathSurface* surf);
	};

	/// Create a new path which is a reflection of this one around the given plane
	Path* reflectPath(const Path* path, const MultipathSurface* surf);


	/// Defines the rotation of an object in space, governed by time
	class RotationPath
	{
	public:
		enum InterpType { RS_INTERP_STATIC, RS_INTERP_CONSTANT, RS_INTERP_LINEAR, RS_INTERP_CUBIC };

		/// Default constructor
		explicit RotationPath(InterpType type = RS_INTERP_STATIC);

		/// Method to add a co-ordinate to the path
		void addCoord(const RotationCoord& coord);

		/// Finalize the path, so it can be requested with getPosition
		void finalize();

		/// Get the position an object will be at at a specified time
		[[nodiscard]] SVec3 getPosition(RS_FLOAT t) const;

		/// Set the interpolation type
		void setInterp(InterpType setinterp);

		/// Set properties for fixed rate motion
		void setConstantRate(const RotationCoord& setstart, const RotationCoord& setrate);

	protected:
		std::vector<RotationCoord> _coords; //!< Vector of time and space coordinates
		std::vector<RotationCoord> _dd; //!< Vector of second derivatives at points (used for cubic interpolation)
		bool _final; //!< Is the path finalised?

		RotationCoord _start; //!< Start point of constant rotation
		RotationCoord _rate; //!< Rotation rate of constant rotation (rads per sec)

		InterpType _type; //!< Type of interpolation
		/// Create a new path which is a reflection of this one around the given plane
		friend RotationPath* reflectPath(const RotationPath* path, const MultipathSurface* surf);
	};

	/// Create a new path which is a reflection of this one around the given plane
	RotationPath* reflectPath(const RotationPath* path, const MultipathSurface* surf);

	/// Exception class for the path code
	class PathException final : public std::runtime_error
	{
	public:
		explicit PathException(const std::string& description):
			std::runtime_error("Error While Executing Path Code: " + description)
		{
		}
	};
}

namespace rs
{
	Path* reflectPath(const Path* path, const MultipathSurface* surf);

	RotationPath* reflectPath(const RotationPath* path, const MultipathSurface* surf);
}

#endif
