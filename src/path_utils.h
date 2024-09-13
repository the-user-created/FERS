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

//The interpolation functions are implemented as template functions to ease adding more functions to both rotation and motion classes

//Static "interpolation" function - the path is at the same point all the time
template <typename T>
void getPositionStatic(T& coord, const std::vector<T>& coords)
{
	if (coords.empty()) { throw path::PathException("coord list empty during GetPositionStatic"); }
	coord = coords[0];
}

//Linear interpolation function
template <typename T>
void getPositionLinear(RS_FLOAT t, T& coord, const std::vector<T>& coords)
{
	T s_key;
	s_key = 0;
	s_key.t = t;
	typename std::vector<T>::const_iterator xrp;
	xrp = upper_bound(coords.begin(), coords.end(), s_key);
	//Check if we are over one of the end points
	if (xrp == coords.begin())
	{
		coord = *xrp; //We are at the left endpoint
	}
	else if (xrp == coords.end())
	{
		coord = *(xrp - 1); //We are at the right endpoint
	}
	else
	{
		//We are at neither endpoint - perform linear interpolation
		int xri = xrp - coords.begin();
		int xli = xri - 1;

		RS_FLOAT iw = coords[xri].t - coords[xli].t;
		RS_FLOAT rw = (coords[xri].t - t) / iw;
		RS_FLOAT lw = 1 - rw;
		//Insert the interpolated values in coord
		coord = coords[xri] * lw + coords[xli] * rw;
	}
	//Set the time part of the coordinate
	coord.t = t;
}

//Cubic spline interpolation function
//The method used (but not the code) is from
//Numerical Recipes in C, Second Edition by Press, et al. pages 114-116
template <typename T>
void getPositionCubic(RS_FLOAT t, T& coord, const std::vector<T>& coords, const std::vector<T>& dd)
{
	T s_key;
	s_key = 0;
	s_key.t = t;
	typename std::vector<T>::const_iterator xrp;
	//Check that we are finalized, if not, complain
	xrp = upper_bound(coords.begin(), coords.end(), s_key);
	//Check if we are over one of the end points
	if (xrp == coords.begin())
	{
		coord = *xrp; //We are at the left endpoint
	}
	else if (xrp == coords.end())
	{
		coord = *(xrp - 1); //We are at the right endpoint
	}
	else
	{
		//We are at neither endpoint - perform cubic spline interpolation
		int xri = xrp - coords.begin();
		int xli = xri - 1;
		const RS_FLOAT xrd = coords[xri].t - t, xld = t - coords[xli].t, iw = coords[xri].t - coords[xli].t, iws = iw *
			               iw / 6.0;
		RS_FLOAT a = xrd / iw, b = xld / iw, c = (a * a * a - a) * iws, d = (b * b * b - b) * iws;
		coord = coords[xli] * a + coords[xri] * b + dd[xli] * c + dd[xri] * d;
	}
	//Set the time part of the coordinate
	coord.t = t;
}

//Finalize function to calculate vector of second derivatives
//The method used (but not the code) is from
//Numerical Recipes in C, Second Edition by Press, et al. pages 114-116
template <typename T>
void finalizeCubic(std::vector<T>& coords, std::vector<T>& dd)
{
	int size = coords.size();
	std::vector<T> tmp(size);
	dd.resize(size);

	//Set the second derivative at the end points to zero
	dd[0] = 0;
	dd[size - 1] = 0;
	//Forward pass of calculating the second derivatives at each point
	for (int i = 1; i < size - 1; i++)
	{
		T yrd = coords[i + 1] - coords[i], yld = coords[i] - coords[i - 1];
		RS_FLOAT xrd = coords[i + 1].t - coords[i].t, xld = coords[i].t - coords[i - 1].t;
		RS_FLOAT iw = coords[i + 1].t - coords[i - 1].t;
		RS_FLOAT si = xld / iw;
		T p = dd[i - 1] * si + 2.0;
		dd[i] = (si - 1.0) / p;
		tmp[i] = ((yrd / xrd - yld / xld) * 6.0 / iw - tmp[i - 1] * si) / p;
	}
	//Second (backward) pass of calculation
	for (int i = size - 2; i >= 0; --i) { dd[i] = dd[i] * dd[i + 1] + tmp[i]; }
}

#endif //PATH_UTILS_H
