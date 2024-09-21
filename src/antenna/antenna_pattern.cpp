// antenna_pattern.cpp
// Implementation of interpolated 2D arrays for gain patterns and RCS patterns
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 19 September 2007

#include "antenna_pattern.h"

#include <cmath>

#include "config.h"
#include "serialization/hdf5_handler.h"

using namespace rs;

constexpr RS_FLOAT TWO_PI = 2.0 * M_PI; // Constant for 2*π

RS_FLOAT Pattern::getGain(const SVec3& angle) const
{
	// Normalizing the azimuth and elevation angles between 0 and 1
	const double ex1 = (angle.azimuth + M_PI) / TWO_PI;
	const double ey1 = (angle.elevation + M_PI) / TWO_PI;

	// Floor-based grid points for interpolation
	const double x1 = std::floor(ex1 * (_size_azi - 1)) / (_size_azi - 1);
	const double x2 = std::min(x1 + 1.0 / _size_azi, 1.0); // Avoid exceeding the range
	const double y1 = std::floor(ey1 * (_size_elev - 1)) / (_size_elev - 1);
	const double y2 = std::min(y1 + 1.0 / _size_elev, 1.0); // Avoid exceeding the range

	// Interpolation weights
	const double t = (ex1 - x1) / (x2 - x1);
	const double u = (ey1 - y1) / (y2 - y1);

	// Array indices for pattern lookup, ensuring they never exceed the array size
	const unsigned arr_x = std::min(static_cast<unsigned>(std::floor(x1 * _size_azi)), _size_azi - 1);
	const unsigned arr_y = std::min(static_cast<unsigned>(std::floor(y1 * _size_elev)), _size_elev - 1);

	// Bilinear interpolation using precomputed indices and weights
	RS_FLOAT interp = (1.0 - t) * (1.0 - u) * _pattern[arr_x][arr_y];
	interp += t * (1.0 - u) * _pattern[(arr_x + 1) % _size_azi][arr_y];
	interp += t * u * _pattern[(arr_x + 1) % _size_azi][(arr_y + 1) % _size_elev];
	interp += (1.0 - t) * u * _pattern[arr_x][(arr_y + 1) % _size_elev];

	return interp;
}
