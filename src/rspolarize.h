// Polarization support classes
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//31 March 2008

#ifndef RS_POLARIZE_H
#define RS_POLARIZE_H

#include <complex>
#include "config.h"

namespace rs
{
	///Polarization Scattering Matrix, implements matrices in the Jones calculus
	class PsMatrix
	{
	public:
		///Default constructor creates identity PSM
		PsMatrix();

		/// Constructor
		PsMatrix(RS_FLOAT s11, RS_FLOAT s12, RS_FLOAT s21, RS_FLOAT s22);

		/// Copy constructor
		PsMatrix(const PsMatrix& im);

		/// Assignment operator
		PsMatrix& operator=(const PsMatrix& im);

		/// Matrix values
		std::complex<RS_FLOAT> s[4];
	};

	//Jones vector class
	class JonesVector
	{
	public:
		/// Constructor
		JonesVector(std::complex<RS_FLOAT> h, std::complex<RS_FLOAT> v);

		/// Copy constructor
		JonesVector(const JonesVector& iv);

		/// Assignment operator
		JonesVector& operator=(const JonesVector& iv);

		/// Multiplication operator
		JonesVector operator*(const PsMatrix& mat) const;

		/// The horizontal polarization part
		std::complex<RS_FLOAT> h;
		/// The vertical polarization part
		std::complex<RS_FLOAT> v;
	};

	//Support functions

	/// Dot product of two Jones vectors
	std::complex<RS_FLOAT> dot(const JonesVector& a, const JonesVector& b);
}

#endif
