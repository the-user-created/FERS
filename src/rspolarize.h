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
		PsMatrix(rsFloat s11, rsFloat s12, rsFloat s21, rsFloat s22);

		/// Copy constructor
		PsMatrix(const PsMatrix& im);

		/// Assignment operator
		PsMatrix& operator=(const PsMatrix& im);

		/// Matrix values
		std::complex<rsFloat> s[4];
	};

	//Jones vector class
	class JonesVector
	{
	public:
		/// Constructor
		JonesVector(std::complex<rsFloat> h, std::complex<rsFloat> v);

		/// Copy constructor
		JonesVector(const JonesVector& iv);

		/// Assignment operator
		JonesVector& operator=(const JonesVector& iv);

		/// Multiplication operator
		JonesVector operator*(const PsMatrix& mat) const;

		/// The horizontal polarization part
		std::complex<rsFloat> h;
		/// The vertical polarization part
		std::complex<rsFloat> v;
	};

	//Support functions

	/// Dot product of two Jones vectors
	std::complex<rsFloat> dot(const JonesVector& a, const JonesVector& b);
}

#endif
