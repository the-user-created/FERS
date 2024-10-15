/**
 * @file antenna_pattern.cpp
 * @brief Implementation of interpolated 2D arrays for gain patterns and RCS patterns.
 *
 * @authors David Young, Marc Brooker
 * @date 2007-09-19
 */

#include "antenna_pattern.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "core/logging.h"
#include "math/geometry_ops.h"

constexpr RealType TWO_PI = 2.0 * PI; // Constant for 2*π

namespace antenna
{
	RealType Pattern::getGain(const math::SVec3& angle) const noexcept
	{
		const double ex1 = (angle.azimuth + PI) / TWO_PI;
		const double ey1 = (angle.elevation + PI) / TWO_PI;

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

		const double t = (ex1 - x1) / (x2 - x1);
		const double u = (ey1 - y1) / (y2 - y1);

		const auto calc_array_index = [](const double value, const unsigned size)
		{
			return std::min(static_cast<unsigned>(std::floor(value * size)), size - 1);
		};

		const unsigned arr_x = calc_array_index(x1, size_azi);
		const unsigned arr_y = calc_array_index(y1, size_elev);

		const RealType interp =
			(1.0 - t) * (1.0 - u) * _pattern[arr_x][arr_y] +
			t * (1.0 - u) * _pattern[(arr_x + 1) % size_azi][arr_y] +
			t * u * _pattern[(arr_x + 1) % size_azi][(arr_y + 1) % size_elev] +
			(1.0 - t) * u * _pattern[arr_x][(arr_y + 1) % size_elev];

		return interp;
	}
}
