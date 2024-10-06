// polarization_matrix.h
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#pragma once

#include <array>

#include "config.h"

namespace math
{
	class PsMatrix
	{
	public:
		PsMatrix() noexcept;

		PsMatrix(RealType s11, RealType s12, RealType s21, RealType s22) noexcept;

		PsMatrix(const PsMatrix&) = default;

		PsMatrix& operator=(const PsMatrix&) = default;

		~PsMatrix() = default;

		std::array<ComplexType, 4> s{};
	};
}
