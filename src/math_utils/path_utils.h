// path_utils.h
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <algorithm>

namespace math
{
	class PathException final : public std::runtime_error
	{
	public:
		explicit PathException(const std::string& description)
			: std::runtime_error("Error While Executing Path Code: " + description) {}
	};
}

// Define a concept to ensure type T supports the necessary operations
template <typename T>
concept Interpolatable = requires(T a, T b, RealType t)
{
	{ a - b } -> std::same_as<T>;
	{ a * t } -> std::same_as<T>;
	{ a + b } -> std::same_as<T>;
	{ a.t } -> std::convertible_to<RealType>;
};

// The interpolation functions are implemented as template functions
// to ease adding more functions to both rotation and motion classes

// Static interpolation: the value remains constant
template <Interpolatable T>
void getPositionStatic(T& coord, const std::vector<T>& coords)
{
	if (coords.empty()) { throw math::PathException("coord list empty during GetPositionStatic"); }
	coord = coords[0];
}

// Linear interpolation: interpolates linearly between two points
template <Interpolatable T>
void getPositionLinear(RealType t, T& coord, const std::vector<T>& coords)
{
	if (coords.empty()) { throw math::PathException("coord list empty during GetPositionLinear"); }

	// Use std::ranges and algorithms to simplify the search
	auto xrp = std::ranges::upper_bound(coords, t, {}, &T::t);

	if (xrp == coords.begin()) { coord = *xrp; }
	else if (xrp == coords.end()) { coord = *(xrp - 1); }
	else
	{
		auto xri = std::distance(coords.begin(), xrp);
		auto xli = xri - 1;
		RealType iw = coords[xri].t - coords[xli].t;
		RealType rw = (coords[xri].t - t) / iw;
		RealType lw = 1 - rw;
		coord = coords[xri] * lw + coords[xli] * rw;
	}
	coord.t = t;
}

// Cubic spline interpolation using second derivatives
// The method used (but not the code) is from
// Numerical Recipes in C, Second Edition by Press, et al. pages 114-116
template <Interpolatable T>
void getPositionCubic(RealType t, T& coord, const std::vector<T>& coords, const std::vector<T>& dd)
{
	if (coords.empty()) { throw math::PathException("coord list empty during GetPositionCubic"); }

	auto xrp = std::ranges::upper_bound(coords, t, {}, &T::t);

	if (xrp == coords.begin()) { coord = *xrp; }
	else if (xrp == coords.end()) { coord = *(xrp - 1); }
	else
	{
		auto xri = std::distance(coords.begin(), xrp);
		auto xli = xri - 1;
		const RealType xrd = coords[xri].t - t;
		const RealType xld = t - coords[xli].t;
		const RealType iw = coords[xri].t - coords[xli].t;
		const RealType iws = iw * iw / 6.0;
		RealType a = xrd / iw;
		RealType b = xld / iw;
		RealType c = (a * a * a - a) * iws;
		RealType d = (b * b * b - b) * iws;
		coord = coords[xli] * a + coords[xri] * b + dd[xli] * c + dd[xri] * d;
	}
	coord.t = t;
}

// Finalize cubic interpolation: Calculate second derivatives
// The method used (but not the code) is from
// Numerical Recipes in C, Second Edition by Press, et al. pages 114-116
template <Interpolatable T>
void finalizeCubic(std::vector<T>& coords, std::vector<T>& dd)
{
	const int size = coords.size();
	if (size < 2) { throw math::PathException("Not enough points for cubic interpolation"); }

	std::vector<T> tmp(size);
	dd.resize(size);

	dd[0] = 0;
	dd[size - 1] = 0;

	for (int i = 1; i < size - 1; ++i)
	{
		T yrd = coords[i + 1] - coords[i];
		T yld = coords[i] - coords[i - 1];
		RealType xrd = coords[i + 1].t - coords[i].t;
		RealType xld = coords[i].t - coords[i - 1].t;
		RealType iw = coords[i + 1].t - coords[i - 1].t;
		RealType si = xld / iw;
		T p = dd[i - 1] * si + 2.0;
		dd[i] = (si - 1.0) / p;
		tmp[i] = ((yrd / xrd - yld / xld) * 6.0 / iw - tmp[i - 1] * si) / p;
	}

	for (int i = size - 2; i >= 0; --i) { dd[i] = dd[i] * dd[i + 1] + tmp[i]; }
}

#endif //PATH_UTILS_H
