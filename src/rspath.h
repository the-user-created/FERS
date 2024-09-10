//rspath.h
//Structures and Classes for Paths and Coordinates
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 21 April 2006

#ifndef RS_PATH_H
#define RS_PATH_H

#include <config.h>
#include <stdexcept>
#include <vector>

#include "rsgeometry.h"

//Forward declarations
namespace rsPython
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
		rsFloat t; //!< Time

		/// Less than comparison operator. Operates only on time value.
		bool operator<(const Coord b) const
		{
			return t < b.t;
		}

		/// Assignment operator
		Coord& operator=(const rsFloat a)
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
	Coord operator+(const Coord& a, rsFloat b);

	/// Componentwise multiplication by scalar
	Coord operator*(const Coord& a, rsFloat b);

	/// Componentwise division by scalar
	Coord operator/(rsFloat a, const Coord& b);

	Coord operator/(const Coord& b, rsFloat a);

	/// Azimuth-Elevation rotation position and time
	//Note that the plane for azimuth is the x-y plane (see Coord)
	struct RotationCoord
	{
		rsFloat azimuth; //!< Angle in X-Y plane (radians)
		rsFloat elevation; //!< Elevation above X-Y plane (radians)
		rsFloat t; //!< Time

		/// Less than comparison operator. Operates only on time value.
		bool operator<(const RotationCoord b) const
		{
			return t < b.t;
		}

		/// Assignment operator
		RotationCoord& operator=(const rsFloat a)
		{
			azimuth = elevation = t = a;
			return *this;
		}

		/// Constructor. Assign scalar to all elements.
		explicit RotationCoord(const rsFloat a = 0):
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
	RotationCoord operator+(const RotationCoord& a, rsFloat b);

	/// Multiply each component by a scalar
	RotationCoord operator*(const RotationCoord& a, rsFloat b);

	/// Divide each component by a scalar
	RotationCoord operator/(rsFloat a, const RotationCoord& b);

	RotationCoord operator/(const RotationCoord& b, rsFloat a);

	/// Defines the movement of an object through space, governed by time
	class Path
	{
	public:
		enum InterpType { RS_INTERP_STATIC, RS_INTERP_LINEAR, RS_INTERP_CUBIC, RS_INTERP_PYTHON };

		/// Default constructor
		explicit Path(InterpType type = RS_INTERP_STATIC);

		/// Add a co-ordinate to the path
		void AddCoord(const Coord& coord);

		/// Finalize the path, so it can be requested with getPosition
		void Finalize();

		/// Get the position an object will be at at a specified time
		Vec3 GetPosition(rsFloat t) const;

		/// Set the interpolation type of the path
		void SetInterp(InterpType settype);

		/// Load a python path function
		void LoadPythonPath(const std::string& modname, const std::string& pathname);

	private:
		std::vector<Coord> coords; //!< Vector of time and space coordinates
		std::vector<Coord> dd; //!< Vector of second derivatives at points (used for cubic interpolation)
		bool final; //!< Is the path finalised?
		InterpType type; //!< Type of interpolation
		rsPython::PythonPath* pythonpath; //!< Pointer to the PythonPath object
		/// Create a new path which is a reflection of this one around the given plane
		friend Path* ReflectPath(const Path* path, const MultipathSurface* surf);
	};

	/// Create a new path which is a reflection of this one around the given plane
	Path* ReflectPath(const Path* path, const MultipathSurface* surf);


	/// Defines the rotation of an object in space, governed by time
	class RotationPath
	{
	public:
		enum InterpType { RS_INTERP_STATIC, RS_INTERP_CONSTANT, RS_INTERP_LINEAR, RS_INTERP_CUBIC };

		/// Default constructor
		explicit RotationPath(InterpType type = RS_INTERP_STATIC);

		/// Method to add a co-ordinate to the path
		void AddCoord(const RotationCoord& coord);

		/// Finalize the path, so it can be requested with getPosition
		void Finalize();

		/// Get the position an object will be at at a specified time
		SVec3 GetPosition(rsFloat t) const;

		/// Set the interpolation type
		void SetInterp(InterpType setinterp);

		/// Set properties for fixed rate motion
		void SetConstantRate(const RotationCoord& setstart, const RotationCoord& setrate);

	protected:
		std::vector<RotationCoord> coords; //!< Vector of time and space coordinates
		std::vector<RotationCoord> dd; //!< Vector of second derivatives at points (used for cubic interpolation)
		bool final; //!< Is the path finalised?

		RotationCoord start; //!< Start point of constant rotation
		RotationCoord rate; //!< Rotation rate of constant rotation (rads per sec)

		InterpType type; //!< Type of interpolation
		/// Create a new path which is a reflection of this one around the given plane
		friend RotationPath* ReflectPath(const RotationPath* path, const MultipathSurface* surf);
	};

	/// Create a new path which is a reflection of this one around the given plane
	RotationPath* ReflectPath(const RotationPath* path, const MultipathSurface* surf);

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
	Path* ReflectPath(const Path* path, const MultipathSurface* surf);

	RotationPath* ReflectPath(const RotationPath* path, const MultipathSurface* surf);
}

#endif
