// Implementation of polarization functions
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//31 March 2008

#include "rspolarize.h"

using namespace rs;

//
// JonesVector Implementation
//
/// Constructor
JonesVector::JonesVector(const std::complex<rsFloat> h, const std::complex<rsFloat> v):
	h(h),
	v(v)
{
}

/// Copy constructor
JonesVector::JonesVector(const JonesVector& iv):
	h(iv.h),
	v(iv.v)
{
}

/// Assignment operator
JonesVector& JonesVector::operator=(const JonesVector& iv)
{
	v = iv.v;
	h = iv.h;
	return *this;
}

/// Multiplication operator
JonesVector JonesVector::operator*(const PSMatrix& mat) const
{
	JonesVector j(h * mat.s[0] + v * mat.s[1], h * mat.s[2] + v * mat.s[3]);
	return j;
}

//
// PSMatrix Implementation
//

///Default constructor creates identity PSM
PSMatrix::PSMatrix()
{
	s[0] = s[3] = 1;
	s[1] = s[2] = 0;
}

/// Constructor
PSMatrix::PSMatrix(const rsFloat s11, const rsFloat s12, const rsFloat s21, const rsFloat s22)
{
	s[0] = s11;
	s[1] = s12;
	s[2] = s21;
	s[3] = s22;
}

/// Copy constructor
PSMatrix::PSMatrix(const PSMatrix& im)
{
	for (int i = 0; i < 4; i++)
	{
		s[i] = im.s[i];
	}
}

/// Assignment operator
PSMatrix& PSMatrix::operator=(const PSMatrix& im)
{
	for (int i = 0; i < 4; i++)
	{
		s[i] = im.s[i];
	}
	return *this;
}

/// Dot product of two Jones vectors
std::complex<rsFloat> dot(const JonesVector& a, const JonesVector& b)
{
	return a.v * b.v + a.h * b.h;
}
