// antenna_pattern.cpp
// Implementation of interpolated 2D arrays for gain patterns and RCS patterns
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 19 September 2007

#include "antenna_pattern.h"

#include <algorithm>                  // for min
#include <cmath>                      // for floor
#include <utility>                    // for pair

#include "core/logging.h"
#include "math_utils/geometry_ops.h"  // for SVec3

constexpr RealType TWO_PI = 2.0 * PI; // Constant for 2*π

namespace antenna
{
	RealType Pattern::getGain(const math::SVec3& angle) const
	{
		// Normalizing the azimuth and elevation angles between 0 and 1
		const double ex1 = (angle.azimuth + PI) / TWO_PI;
		const double ey1 = (angle.elevation + PI) / TWO_PI;

		// Calculate floor-based grid points for interpolation
		const auto calc_grid_point = [](const double value, const unsigned size)
		{
			const double x1 = std::floor(value * (size - 1)) / (size - 1);
			const double x2 = std::min(x1 + 1.0 / size, 1.0);
			return std::pair{x1, x2};
		};

		unsigned size_azi = _pattern.size();
		unsigned size_elev = _pattern[0].size();

		LOG(logging::Level::TRACE, "Size of pattern: {} x {}", size_azi, size_elev);

		const auto [x1, x2] = calc_grid_point(ex1, size_azi);
		const auto [y1, y2] = calc_grid_point(ey1, size_elev);

		// Interpolation weights
		const double t = (ex1 - x1) / (x2 - x1);
		const double u = (ey1 - y1) / (y2 - y1);

		// Calculate array indices, ensuring they don't exceed the array bounds
		const auto calc_array_index = [](const double value, const unsigned size)
		{
			return std::min(static_cast<unsigned>(std::floor(value * size)), size - 1);
		};

		const unsigned arr_x = calc_array_index(x1, size_azi);
		const unsigned arr_y = calc_array_index(y1, size_elev);

		// Bilinear interpolation using precomputed indices and weights
		const RealType interp =
			(1.0 - t) * (1.0 - u) * _pattern[arr_x][arr_y] +
			t * (1.0 - u) * _pattern[(arr_x + 1) % size_azi][arr_y] +
			t * u * _pattern[(arr_x + 1) % size_azi][(arr_y + 1) % size_elev] +
			(1.0 - t) * u * _pattern[arr_x][(arr_y + 1) % size_elev];

		return interp;
	}
}
