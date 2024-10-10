/**
 * @file polarization_matrix.cpp
 * @brief Contains the implementation of the PsMatrix class.
 *
 * @authors David Young, Marc Brooker
 * @date 2024-09-12
 */

#include "polarization_matrix.h"

#include <complex>

namespace math
{
	// Default constructor
	PsMatrix::PsMatrix() noexcept
	{
		s[0] = {1, 0}; // Initialize as (1 + 0i)
		s[1] = {0, 0};
		s[2] = {0, 0};
		s[3] = {1, 0};
	}

	// Parameterized constructor
	PsMatrix::PsMatrix(RealType s11, RealType s12, RealType s21, RealType s22) noexcept
	{
		s[0] = {s11, 0};
		s[1] = {s12, 0};
		s[2] = {s21, 0};
		s[3] = {s22, 0};
	}
}
