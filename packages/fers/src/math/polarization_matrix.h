/**
* @file polarization_matrix.h
* @brief Defines the PsMatrix class for polarization matrix operations.
*
* @authors David Young, Marc Brooker
* @date 2024-09-12
*/

#pragma once

#include <array>

#include "config.h"

namespace math
{
	/**
	* @class PsMatrix
	* @brief A class representing a 2x2 polarization scattering matrix.
	*/
	class PsMatrix
	{
	public:
		PsMatrix() noexcept;

		/**
		* @brief Parameterized constructor to initialize the matrix with real values.
		*
		* @param s11 Real part of the matrix element at position (1, 1).
		* @param s12 Real part of the matrix element at position (1, 2).
		* @param s21 Real part of the matrix element at position (2, 1).
		* @param s22 Real part of the matrix element at position (2, 2).
		*/
		PsMatrix(RealType s11, RealType s12, RealType s21, RealType s22) noexcept;

		PsMatrix(const PsMatrix&) = default;
		PsMatrix& operator=(const PsMatrix&) = default;
		~PsMatrix() = default;

		/**
		* @brief Array storing the elements of the matrix.
		*/
		std::array<ComplexType, 4> s{};
	};
}
