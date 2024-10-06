// config.h
// Used for global configuration of the project
// Created by David Young on 9/10/24.
//

#pragma once

#include <complex>

using RealType = double; // Type for real numbers
using ComplexType = std::complex<RealType>; // Type for complex numbers
constexpr RealType PI = std::numbers::pi_v<RealType>; // Constant for π
constexpr RealType EPSILON = std::numeric_limits<RealType>::epsilon(); // Machine epsilon
