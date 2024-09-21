// polarization_matrix.cpp
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "polarization_matrix.h"

namespace math
{
	PsMatrix::PsMatrix()
	{
		s[0] = s[3] = 1;
		s[1] = s[2] = 0;
	}

	PsMatrix::PsMatrix(const RealType s11, const RealType s12, const RealType s21, const RealType s22)
	{
		s[0] = s11;
		s[1] = s12;
		s[2] = s21;
		s[3] = s22;
	}

	PsMatrix::PsMatrix(const PsMatrix& im)
	{
		for (int i = 0; i < 4; i++)
		{
			s[i] = im.s[i];
		}
	}

	PsMatrix& PsMatrix::operator=(const PsMatrix& im)
	{
		if (this != &im)
		{
			for (int i = 0; i < 4; i++)
			{
				s[i] = im.s[i];
			}
		}
		return *this;
	}
}
