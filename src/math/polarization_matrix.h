/**
* @file polarization_matrix.h
* @brief Defines the PsMatrix class for polarization matrix operations.
*
* This file contains the declaration of the PsMatrix class,
* which is used to represent and manipulate a polarization scattering matrix.
* The class supports default initialization as well as initialization with specific real components for the matrix elements.
* This matrix can be used in various mathematical or physical computations,
* particularly in the context of electromagnetic wave scattering and polarization.
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
	*
	* The PsMatrix class stores a 2x2 complex polarization scattering matrix, with support for
	* default construction (identity matrix) and parameterized construction with real values.
	* This matrix can be used for representing scattering parameters in electromagnetic simulations
	* and polarization studies.
	* Each element of the matrix is represented as a complex number.
	*/
	class PsMatrix
	{
	public:
		/**
		* @brief Default constructor that initializes the matrix to an identity matrix.
		*
		* Initializes the polarization matrix with complex numbers representing an identity matrix:
		* s11 = (1 + 0i), s12 = (0 + 0i), s21 = (0 + 0i), s22 = (1 + 0i).
		*/
		PsMatrix() noexcept;

		/**
		* @brief Parameterized constructor to initialize the matrix with real values.
		*
		* This constructor allows the user to specify the real components of the matrix.
		* The imaginary components are set to zero.
		*
		* @param s11 Real part of the matrix element at position (1, 1).
		* @param s12 Real part of the matrix element at position (1, 2).
		* @param s21 Real part of the matrix element at position (2, 1).
		* @param s22 Real part of the matrix element at position (2, 2).
		*/
		PsMatrix(RealType s11, RealType s12, RealType s21, RealType s22) noexcept;

		/**
		* @brief Default copy constructor.
		*
		* Copies the elements of another PsMatrix.
		*/
		PsMatrix(const PsMatrix&) = default;

		/**
		* @brief Default assignment operator.
		*
		* Assigns the values of another PsMatrix to this one.
		*
		* @return A reference to this PsMatrix object.
		*/
		PsMatrix& operator=(const PsMatrix&) = default;

		/**
		* @brief Default destructor.
		*
		* Destructor for cleaning up PsMatrix resources (if any).
		*/
		~PsMatrix() = default;

		/**
		* @brief Array storing the elements of the matrix.
		*
		* The array stores four complex numbers corresponding to the 2x2 matrix elements:
		* s[0] = s11, s[1] = s12, s[2] = s21, s[3] = s22.
		*/
		std::array<ComplexType, 4> s{};
	};
}
