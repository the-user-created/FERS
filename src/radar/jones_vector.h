// jones_vector.h
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef JONES_VECTOR_H
#define JONES_VECTOR_H

#include <complex>

#include "config.h"
#include "math_utils/polarization_matrix.h"

namespace rs
{
	class JonesVector
	{
	public:
		JonesVector(const std::complex<RS_FLOAT> h, const std::complex<RS_FLOAT> v) : h(h), v(v)
		{
		}

		JonesVector(const JonesVector& iv) = default;

		JonesVector& operator=(const JonesVector& iv) = default;

		JonesVector operator*(const PsMatrix& mat) const
		{
			return {h * mat.s[0] + v * mat.s[1], h * mat.s[2] + v * mat.s[3]};
		}

		std::complex<RS_FLOAT> h;
		std::complex<RS_FLOAT> v;
	};

	inline std::complex<RS_FLOAT> dot(const JonesVector& a, const JonesVector& b)
	{
		return a.v * b.v + a.h * b.h;
	}
}

#endif //JONES_VECTOR_H
