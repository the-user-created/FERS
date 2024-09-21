// polarization_matrix.h
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef POLARIZATION_MATRIX_H
#define POLARIZATION_MATRIX_H

#include "config.h"

namespace math
{
	class PsMatrix
	{
	public:
		PsMatrix();

		PsMatrix(RealType s11, RealType s12, RealType s21, RealType s22);

		PsMatrix(const PsMatrix& im);

		PsMatrix& operator=(const PsMatrix& im);

		ComplexType s[4];
	};
}

#endif //POLARIZATION_MATRIX_H
