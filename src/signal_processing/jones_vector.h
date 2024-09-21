// jones_vector.h
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef JONES_VECTOR_H
#define JONES_VECTOR_H

#include "math_utils/polarization_matrix.h"

namespace signal
{
	class JonesVector
	{
	public:
		JonesVector(const ComplexType h, const ComplexType v) : _h(h), _v(v)
		{
		}

		JonesVector(const JonesVector& iv) = default;

		JonesVector& operator=(const JonesVector& iv) = default;

		JonesVector operator*(const math::PsMatrix& mat) const
		{
			return {_h * mat.s[0] + _v * mat.s[1], _h * mat.s[2] + _v * mat.s[3]};
		}

		[[nodiscard]] ComplexType getH() const
		{
			return _h;
		}

		[[nodiscard]] ComplexType getV() const
		{
			return _v;
		}

	private:
		ComplexType _h;
		ComplexType _v;
	};

	// Note: This function is not used in the codebase
	inline ComplexType dot(const JonesVector& a, const JonesVector& b)
	{
		return a.getV() * b.getV() + a.getH() * b.getH();
	}
}

#endif //JONES_VECTOR_H
