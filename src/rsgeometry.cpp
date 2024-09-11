//rsgeometry.cpp
//Implementation of geometry classes
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//26 May 2006

#include "rsgeometry.h"

#include <cmath>

using namespace rs;

//
//Vec3 Implementation
//

//Default constructor
Vec3::Vec3():
	x(0), y(0), z(0)
{
}

//Data initialization constructor
Vec3::Vec3(const RS_FLOAT x, const RS_FLOAT y, const RS_FLOAT z):
	x(x), y(y), z(z)
{
}

//Default destructor
Vec3::~Vec3() = default;

//Constructor with a spherical vector
Vec3::Vec3(const SVec3& svec)
{
	x = svec.length * std::cos(svec.azimuth) * std::cos(svec.elevation);
	y = svec.length * std::sin(svec.azimuth) * std::cos(svec.elevation);
	z = svec.length * std::sin(svec.elevation);
}

//Addition operator
Vec3& Vec3::operator+=(const Vec3& b)
{
	x += b.x;
	y += b.y;
	z += b.z;
	return *this;
}

//Subtraction operator
Vec3& Vec3::operator-=(const Vec3& b)
{
	x -= b.x;
	y -= b.y;
	z -= b.z;
	return *this;
}

//Componentwise multiplication
//Used by interpolation code. See DotProduct and CrossProduct for more sensible products
Vec3& Vec3::operator*=(const Vec3& b)
{
	x *= b.x;
	y *= b.y;
	z *= b.z;
	return *this;
}

//Assignment operator
Vec3& Vec3::operator=(const Vec3& b) = default;

// Multiplication by a 3x3 matrix
Vec3& Vec3::operator*=(const Matrix3& m)
{
	const RS_FLOAT* mat = m.getData();
	const Vec3 v(x, y, z);
	x = mat[0] * v.x + mat[1] * v.y + mat[2] * v.z;
	y = mat[3] * v.x + mat[4] * v.y + mat[5] * v.z;
	z = mat[6] * v.x + mat[7] * v.y + mat[8] * v.z;
	return *this;
}

//Length division operator
Vec3& Vec3::operator/=(const RS_FLOAT b)
{
	x /= b;
	y /= b;
	z /= b;
	return *this;
}

//Scaling operator
Vec3& Vec3::operator*=(const RS_FLOAT b)
{
	x *= b;
	y *= b;
	z *= b;
	return *this;
}

//Addition of a scalar
Vec3& Vec3::operator+=(const RS_FLOAT b)
{
	x += b;
	y += b;
	z += b;
	return *this;
}

//Return the length of the vector
RS_FLOAT Vec3::length() const
{
	return sqrt(x * x + y * y + z * z);
}

//
// Vec3 Non-member functions
//

//Inner (dot) product operator
RS_FLOAT rs::dotProduct(const Vec3& a, const Vec3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

//Returns a new vector containing the cross (outer) product of two vectors
Vec3 rs::crossProduct(const Vec3& a, const Vec3& b)
{
	Vec3 newvec;
	newvec.x = a.y * b.z - a.z * b.y;
	newvec.y = a.z * b.x - a.x * b.z;
	newvec.z = a.x * b.y - a.y * b.x;
	return newvec;
}

//Componentwise multiplication
Vec3 rs::operator*(const Vec3& a, const Vec3& b)
{
	Vec3 c(a);
	c *= b;
	return c;
}

//Componentwise addition
Vec3 rs::operator+(const Vec3& a, const Vec3& b)
{
	Vec3 c(a);
	c += b;
	return c;
}

//Componentwise subtraction
Vec3 rs::operator-(const Vec3& a, const Vec3& b)
{
	Vec3 c(a);
	c -= b;
	return c;
}

//Componentwise division
Vec3 rs::operator/(const Vec3& a, const Vec3& b)
{
	Vec3 c(a);
	c.x /= b.x;
	c.y /= b.y;
	c.z /= b.z;
	return c;
}

//Multiplication by a scalar
Vec3 rs::operator*(const Vec3& a, const RS_FLOAT b)
{
	Vec3 c(a);
	c *= b;
	return c;
}

//Division by a scalar
Vec3 rs::operator/(const Vec3& a, const RS_FLOAT b)
{
	Vec3 c(a);
	c /= b;
	return c;
}

//Division by a scalar
Vec3 rs::operator/(const RS_FLOAT a, const Vec3& b)
{
	Vec3 c(b);
	c.x = a / c.x;
	c.y = a / c.y;
	c.z = a / c.z;
	return c;
}

//
//SVec3 implementation
//

SVec3::SVec3():
	length(0), azimuth(0), elevation(0)
{
}

//Constructor with initialization
SVec3::SVec3(const RS_FLOAT length, const RS_FLOAT azimuth, const RS_FLOAT elevation):
	length(length), azimuth(azimuth), elevation(elevation)
{
}

//Copy constructor
SVec3::SVec3(const SVec3& svec) = default;

//Destructor
SVec3::~SVec3() = default;

//Constructor with a rectangular vector
SVec3::SVec3(const Vec3& vec)
{
	length = vec.length();
	if (length != 0)
	{
		elevation = std::asin(vec.z / length);
		azimuth = std::atan2(vec.y, vec.x);
	}
	else
	{
		elevation = 0;
		azimuth = 0;
	}
}

//Multiplication by a scalar
SVec3& SVec3::operator*=(const RS_FLOAT b)
{
	length *= b;
	return *this;
}

//Division by a scalar
SVec3& SVec3::operator/=(const RS_FLOAT b)
{
	length /= b;
	return *this;
}

//
// Matrix3 Implementation
//

/// Default constructor
Matrix3::Matrix3()
{
	for (double & element : elements)
	{
		element = 0;
	}
}

/// Default destructor
Matrix3::~Matrix3() = default;

/// Get a pointer to the element array
const RS_FLOAT* Matrix3::getData() const
{
	return elements;
}

RS_FLOAT* Matrix3::getData()
{
	return elements;
}
