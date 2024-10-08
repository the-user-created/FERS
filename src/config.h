/**
* @file config.h
* @brief Global configuration file for the project.
*
* This file contains type definitions and constants used throughout the project
* for real and complex number types, as well as mathematical constants like π
* and machine epsilon.
*
* @author David Young
* @date 2024-09-10
*/

#pragma once

#include <complex>

/**
* @brief Type for real numbers.
*
* This typedef defines the real number type to be used throughout the project
* as a double-precision floating-point number.
*/
using RealType = double;

/**
* @brief Type for complex numbers.
*
* This typedef defines the complex number type, which is based on the RealType
* definition for double-precision floating-point real and imaginary parts.
*/
using ComplexType = std::complex<RealType>;

/**
* @brief Mathematical constant π (pi).
*
* This constant holds the value of π (pi) using the RealType, ensuring precision
* for calculations involving this constant throughout the project.
*/
constexpr RealType PI = std::numbers::pi_v<RealType>;

/**
* @brief Machine epsilon for real numbers.
*
* This constant defines the smallest representable difference (machine epsilon)
* between two RealType values, ensuring precision control in numerical operations.
*/
constexpr RealType EPSILON = std::numeric_limits<RealType>::epsilon();
