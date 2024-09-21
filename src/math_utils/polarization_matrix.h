// polarization_matrix.h
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef POLARIZATION_MATRIX_H
#define POLARIZATION_MATRIX_H

#include <complex>

#include "config.h"

namespace math
{
	class PsMatrix
	{
	public:
		PsMatrix();

		PsMatrix(RS_FLOAT s11, RS_FLOAT s12, RS_FLOAT s21, RS_FLOAT s22);

		PsMatrix(const PsMatrix& im);

		PsMatrix& operator=(const PsMatrix& im);

		RS_COMPLEX s[4];
	};
}

#endif //POLARIZATION_MATRIX_H
