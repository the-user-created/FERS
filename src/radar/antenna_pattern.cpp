// antenna_pattern.cpp
// Implementation of interpolated 2D arrays for gain patterns and RCS patterns
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 19 September 2007

#include "antenna_pattern.h"

#include <cmath>

#include "config.h"
#include "serialization/hdf5_export.h"

using namespace rs;

RS_FLOAT Pattern::getGain(const SVec3& angle) const
{
	const double x1 = std::floor((angle.azimuth + M_PI) / (2 * M_PI) * (_size_azi - 1)) / (_size_azi - 1);
	const double ex1 = (angle.azimuth + M_PI) / (2 * M_PI);
	const double x2 = x1 + 1.0 / _size_azi;
	const double y1 = std::floor((angle.elevation + M_PI) / (2 * M_PI) * (_size_elev - 1)) / (_size_elev - 1);
	const double ey1 = (angle.elevation + M_PI) / (2 * M_PI);
	const double y2 = y1 + 1.0 / _size_elev;
	const double t = (ex1 - x1) / (x2 - x1);
	const double u = (ey1 - y1) / (y2 - y1);
	const int arr_x = std::floor(x1 * _size_azi);
	const int arr_y = std::floor(y1 * _size_elev);

	RS_FLOAT interp = (1.0 - t) * (1.0 - u) * _pattern[arr_x][arr_y];
	interp += t * (1.0 - u) * _pattern[(arr_x + 1) % _size_azi][arr_y];
	interp += t * u * _pattern[(arr_x + 1) % _size_azi][(arr_y + 1) % _size_elev];
	interp += (1.0 - t) * u * _pattern[arr_x][(arr_y + 1) % _size_elev];
	return interp;
}
