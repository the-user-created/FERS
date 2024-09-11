//rsgeometry.h
//Classes for calculating the geometry of the world
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//26 May 2006

#ifndef RS_GEOMETRY_H
#define RS_GEOMETRY_H

#include <cmath>

#include "config.h"

namespace rs_geometry
{
}

namespace rs
{
	class SVec3;

	/// 3x3 Matrix, with simple operations
	class Matrix3
	{
	public:
		RS_FLOAT elements[9]{};

		/// Default constructor
		Matrix3();

		/// Default destructor
		~Matrix3();

		/// Get a pointer to the element array
		[[nodiscard]] const RS_FLOAT* getData() const;

		RS_FLOAT* getData();
	};

	/// The Vec3 class is a rectangular three vector
	class Vec3
	{
	public:
		RS_FLOAT x, y, z;

		/// Default constructor
		Vec3();

		/// Constructor which sets co-ordinates
		Vec3(RS_FLOAT x, RS_FLOAT y, RS_FLOAT z);

		/// Constructor with a spherical vector
		explicit Vec3(const SVec3& svec);

		/// Default destructor
		~Vec3();

		//
		//operators
		//

		//Vector operations
		Vec3& operator+=(const Vec3& b); //!< Vector Addition
		Vec3& operator-=(const Vec3& b); //!< Vector Subtraction
		Vec3& operator*=(const Vec3& b); //!< Componentwise multiplication
		Vec3& operator=(const Vec3& b); //!< Assignment operator

		//Matrix operations
		Vec3& operator*=(const Matrix3& m); //!< Multiplication by a 3x3 matrix

		//Scalar operations
		Vec3& operator*=(RS_FLOAT b); //!< Multiplication by a scalar
		Vec3& operator/=(RS_FLOAT b); //!< Division by a scalar
		Vec3& operator+=(RS_FLOAT b); //!< Addition of a scalar

		/// Return the length of the vector
		[[nodiscard]] RS_FLOAT length() const;
	};

	//Vector operations
	RS_FLOAT dotProduct(const Vec3& a, const Vec3& b); //!< Vector Inner product
	Vec3 crossProduct(const Vec3& a, const Vec3& b); //!< Vector Cross product
	//Componentwise vector operations
	Vec3 operator*(const Vec3& a, const Vec3& b); //!< Componentwise product
	Vec3 operator+(const Vec3& a, const Vec3& b); //!< Componentwise add
	Vec3 operator-(const Vec3& a, const Vec3& b); //!< Componentwise subtract
	Vec3 operator/(const Vec3& a, const Vec3& b); //!< Componentwise divide
	//Scalar operations
	Vec3 operator*(const Vec3& a, RS_FLOAT b); //!< Multiply by a scalar
	Vec3 operator/(const Vec3& a, RS_FLOAT b); //!< Division by a scalar
	Vec3 operator/(RS_FLOAT a, const Vec3& b); //!< Division by a scalar

	/// The SVec3 class is a vector in R^3, stored in spherical co-ordinates
	class SVec3
	{
	public:
		RS_FLOAT length; //!< The length of the vector
		RS_FLOAT azimuth; //!< Angle in the x-y plane (radians)
		RS_FLOAT elevation; //!< Elevation angle above x-y plane (radians)
		/// Default constructor
		SVec3();

		/// Constructor with initializers
		SVec3(RS_FLOAT length, RS_FLOAT azimuth, RS_FLOAT elevation);

		/// Copy constructor
		SVec3(const SVec3& svec);

		/// Constructor with a rectangular vector
		explicit SVec3(const Vec3& vec);

		/// Destructor
		~SVec3();

		//
		//operators
		//
		SVec3& operator*=(RS_FLOAT b); //!< multiplication by a scalar
		SVec3& operator/=(RS_FLOAT b); //!< division by a scalar
	};

	inline SVec3 operator+(const SVec3& a, const SVec3& b)
	{
		// TODO: verify that this is correct for the FERS use case
		RS_FLOAT new_azimuth = fmod(a.azimuth + b.azimuth, 2 * M_PI);  // Normalize to [0, 2π)
		if (new_azimuth < 0)
		{
			new_azimuth += 2 * M_PI; // Ensure positive azimuth
		}
		RS_FLOAT new_elevation = fmod(a.elevation + b.elevation, M_PI);  // Normalize elevation
		return {a.length + b.length, new_azimuth, new_elevation};
	}

	inline SVec3 operator-(const SVec3& a, const SVec3& b)
	{
		// TODO: verify that this is correct for the FERS use case
		RS_FLOAT new_azimuth = fmod(a.azimuth - b.azimuth, 2 * M_PI);  // Normalize to [0, 2π)
		if (new_azimuth < 0)
		{
			new_azimuth += 2 * M_PI; // Ensure positive azimuth
		}
		RS_FLOAT new_elevation = fmod(a.elevation - b.elevation, M_PI);  // Normalize elevation
		return {a.length - b.length, new_azimuth, new_elevation};
	}

}
#endif
