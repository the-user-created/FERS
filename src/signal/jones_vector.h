// jones_vector.h
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#pragma once

#include "math/polarization_matrix.h"

namespace signal
{
	// NOTE: This class is not used in the codebase
	class JonesVector
	{
	public:
		constexpr JonesVector(const ComplexType h, const ComplexType v) : _h(h), _v(v) {}

		constexpr JonesVector(const JonesVector& iv) = default;

		constexpr JonesVector& operator=(const JonesVector& iv) = default;

		constexpr JonesVector operator*(const math::PsMatrix& mat) const
		{
			return {_h * mat.s[0] + _v * mat.s[1], _h * mat.s[2] + _v * mat.s[3]};
		}

		[[nodiscard]] constexpr ComplexType getH() const { return _h; }
		[[nodiscard]] constexpr ComplexType getV() const { return _v; }

	private:
		ComplexType _h;
		ComplexType _v;
	};

	constexpr ComplexType dot(const JonesVector& a, const JonesVector& b)
	{
		return a.getV() * b.getV() + a.getH() * b.getH();
	}
}
