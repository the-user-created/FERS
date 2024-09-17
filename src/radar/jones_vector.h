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
		JonesVector(const std::complex<RS_FLOAT> h, const std::complex<RS_FLOAT> v) : _h(h), _v(v)
		{
		}

		JonesVector(const JonesVector& iv) = default;

		JonesVector& operator=(const JonesVector& iv) = default;

		JonesVector operator*(const PsMatrix& mat) const
		{
			return {_h * mat.s[0] + _v * mat.s[1], _h * mat.s[2] + _v * mat.s[3]};
		}

		[[nodiscard]] std::complex<RS_FLOAT> getH() const
		{
			return _h;
		}

		[[nodiscard]] std::complex<RS_FLOAT> getV() const
		{
			return _v;
		}

	private:
		std::complex<RS_FLOAT> _h;
		std::complex<RS_FLOAT> _v;
	};

	// Note: This function is not used in the codebase
	inline std::complex<RS_FLOAT> dot(const JonesVector& a, const JonesVector& b)
	{
		return a.getV() * b.getV() + a.getH() * b.getH();
	}
}

#endif //JONES_VECTOR_H
