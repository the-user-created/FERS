// path_utils.h
// Created by David Young on 9/13/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <algorithm>  // NOLINT
#include <config.h>
#include <stdexcept>
#include <vector>

namespace path
{
	class PathException final : public std::runtime_error
	{
	public:
		explicit PathException(const std::string& description)
			: std::runtime_error("Error While Executing Path Code: " + description) {}
	};
}

// TODO: Can use concepts with these template functions

// The interpolation functions are implemented as template functions
// to ease adding more functions to both rotation and motion classes

// Static "interpolation" function - the path is at the same point all the time
template <typename T>
void getPositionStatic(T& coord, const std::vector<T>& coords)
{
	if (coords.empty()) { throw path::PathException("coord list empty during GetPositionStatic"); }
	coord = coords[0];
}

// Linear interpolation function
template <typename T>
void getPositionLinear(RS_FLOAT t, T& coord, const std::vector<T>& coords)
{
	T s_key = {};
	s_key.t = t;
	auto xrp = std::upper_bound(coords.begin(), coords.end(), s_key);

	if (xrp == coords.begin()) { coord = *xrp; }
	else if (xrp == coords.end()) { coord = *(xrp - 1); }
	else
	{
		int xri = xrp - coords.begin();
		int xli = xri - 1;
		RS_FLOAT iw = coords[xri].t - coords[xli].t;
		RS_FLOAT rw = (coords[xri].t - t) / iw;
		RS_FLOAT lw = 1 - rw;
		coord = coords[xri] * lw + coords[xli] * rw;
	}
	coord.t = t;
}

// Cubic spline interpolation function
// The method used (but not the code) is from
// Numerical Recipes in C, Second Edition by Press, et al. pages 114-116
template <typename T>
void getPositionCubic(RS_FLOAT t, T& coord, const std::vector<T>& coords, const std::vector<T>& dd)
{
	T s_key = {};
	s_key.t = t;
	auto xrp = std::upper_bound(coords.begin(), coords.end(), s_key);

	if (xrp == coords.begin()) { coord = *xrp; }
	else if (xrp == coords.end()) { coord = *(xrp - 1); }
	else
	{
		int xri = xrp - coords.begin();
		int xli = xri - 1;
		const RS_FLOAT xrd = coords[xri].t - t, xld = t - coords[xli].t;
		const RS_FLOAT iw = coords[xri].t - coords[xli].t;
		const RS_FLOAT iws = iw * iw / 6.0;
		RS_FLOAT a = xrd / iw, b = xld / iw;
		RS_FLOAT c = (a * a * a - a) * iws;
		RS_FLOAT d = (b * b * b - b) * iws;
		coord = coords[xli] * a + coords[xri] * b + dd[xli] * c + dd[xri] * d;
	}
	coord.t = t;
}

// Finalize function to calculate vector of second derivatives
// The method used (but not the code) is from
// Numerical Recipes in C, Second Edition by Press, et al. pages 114-116
template <typename T>
void finalizeCubic(std::vector<T>& coords, std::vector<T>& dd)
{
	int size = coords.size();
	std::vector<T> tmp(size);
	dd.resize(size);

	dd[0] = 0;
	dd[size - 1] = 0;

	for (int i = 1; i < size - 1; i++)
	{
		T yrd = coords[i + 1] - coords[i];
		T yld = coords[i] - coords[i - 1];
		RS_FLOAT xrd = coords[i + 1].t - coords[i].t;
		RS_FLOAT xld = coords[i].t - coords[i - 1].t;
		RS_FLOAT iw = coords[i + 1].t - coords[i - 1].t;
		RS_FLOAT si = xld / iw;
		T p = dd[i - 1] * si + 2.0;
		dd[i] = (si - 1.0) / p;
		tmp[i] = ((yrd / xrd - yld / xld) * 6.0 / iw - tmp[i - 1] * si) / p;
	}

	for (int i = size - 2; i >= 0; --i) { dd[i] = dd[i] * dd[i + 1] + tmp[i]; }
}

#endif //PATH_UTILS_H
