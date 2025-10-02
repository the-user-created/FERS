// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
* @file polarization_matrix.cpp
* @brief Contains the implementation of the PsMatrix class.
*/

#include "polarization_matrix.h"

#include <complex>

namespace math
{
	PsMatrix::PsMatrix() noexcept
	{
		s[0] = {1, 0};
		s[1] = {0, 0};
		s[2] = {0, 0};
		s[3] = {1, 0};
	}

	PsMatrix::PsMatrix(RealType s11, RealType s12, RealType s21, RealType s22) noexcept
	{
		s[0] = {s11, 0};
		s[1] = {s12, 0};
		s[2] = {s21, 0};
		s[3] = {s22, 0};
	}
}
