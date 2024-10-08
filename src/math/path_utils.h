/**
* @file path_utils.h
 * @brief Utility functions for path interpolation and exception handling.
 *
 * This header file provides a set of functions to handle different types of
 * interpolation (static, linear, and cubic) for coordinate points, and manages
 * exceptions that may occur during these operations.
 * The interpolation functions are implemented as templates and can be used with various types that satisfy
 * the necessary mathematical operations.
 *
 * The cubic interpolation functions are based on methods described in "Numerical Recipes
 * in C, Second Edition" by Press et al., but the code here is distinct from the original.
 *
 * @authors David Young, Marc Brooker
 * @date 2024-09-13
 */

#pragma once

#include <algorithm>

namespace math
{
	/**
	 * @class PathException
	 * @brief Exception class for handling path-related errors.
	 *
	 * This class extends the standard `std::runtime_error` to provide specific
	 * error handling for the path interpolation functions.
	 * It ensures that when an error occurs during interpolation, a meaningful message is generated.
	 */
	class PathException final : public std::runtime_error
	{
	public:
		/**
		 * @brief Constructor for PathException.
		 *
		 * Creates an exception with a specific error message, which is prepended
		 * with a general error description.
		 *
		 * @param description A detailed description of the error.
		 */
		explicit PathException(const std::string& description)
			: std::runtime_error("Error While Executing Path Code: " + description) {}
	};
}

/**
 * @brief Concept for types that can be interpolated.
 *
 * @tparam T The type to be checked for interpolation.
 * @param a The first value to be interpolated.
 * @param b The second value to be interpolated.
 * @param t The interpolation factor.
 */
template <typename T>
concept Interpolatable = requires(T a, T b, RealType t)
{
	{ a - b } -> std::same_as<T>;
	{ a * t } -> std::same_as<T>;
	{ a + b } -> std::same_as<T>;
	{ a.t } -> std::convertible_to<RealType>;
};

/**
 * @brief Interpolates a static position from a list of coordinates.
 *
 * This function sets the provided coordinate to the first element in the list of coordinates.
 * It throws an exception if the list is empty.
 *
 * @tparam T The type of the coordinate, which must satisfy the Interpolatable concept.
 * @param coord The output coordinate to be set.
 * @param coords A vector of coordinates from which the first will be selected.
 * @throws PathException if the list of coordinates is empty.
 */
template <Interpolatable T>
void getPositionStatic(T& coord, const std::vector<T>& coords)
{
	if (coords.empty()) { throw math::PathException("coord list empty during GetPositionStatic"); }
	coord = coords[0];
}

/**
 * @brief Performs linear interpolation between coordinate points.
 *
 * This function interpolates linearly between two points in the list of coordinates based
 * on the value of `t`. It computes a weighted average of the two nearest points and assigns
 * the result to `coord`.
 *
 * @tparam T The type of the coordinate, which must satisfy the Interpolatable concept.
 * @param t The interpolation factor (usually time) to determine the position.
 * @param coord The output coordinate that will be interpolated.
 * @param coords A vector of coordinates to interpolate between.
 * @throws PathException if the list of coordinates is empty.
 */
template <Interpolatable T>
void getPositionLinear(RealType t, T& coord, const std::vector<T>& coords)
{
	if (coords.empty()) { throw math::PathException("coord list empty during GetPositionLinear"); }

	auto xrp = std::ranges::upper_bound(coords, t, {}, &T::t);

	if (xrp == coords.begin()) { coord = *xrp; }
	else if (xrp == coords.end()) { coord = *(xrp - 1); }
	else
	{
		auto xri = std::distance(coords.begin(), xrp);
		auto xli = xri - 1;

		const RealType iw = coords[xri].t - coords[xli].t;
		const RealType rw = (coords[xri].t - t) / iw;
		const RealType lw = 1 - rw;

		coord = coords[xri] * lw + coords[xli] * rw;
	}
	coord.t = t;
}

/**
 * @brief Performs cubic spline interpolation between coordinate points.
 *
 * This function uses cubic spline interpolation to compute a smooth curve between
 * multiple points based on second derivatives provided in `dd`.
 * The method used for calculating the spline is from "Numerical Recipes in C."
 *
 * @tparam T The type of the coordinate, which must satisfy the Interpolatable concept.
 * @param t The interpolation factor (usually time) to determine the position.
 * @param coord The output coordinate that will be interpolated.
 * @param coords A vector of coordinates to interpolate between.
 * @param dd A vector of second derivatives used in the cubic interpolation.
 * @throws PathException if the list of coordinates is empty.
 */
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
		const RealType a = xrd / iw;
		const RealType b = xld / iw;
		const RealType c = (a * a * a - a) * iws;
		const RealType d = (b * b * b - b) * iws;

		coord = coords[xli] * a + coords[xri] * b + dd[xli] * c + dd[xri] * d;
	}
	coord.t = t;
}

/**
 * @brief Finalizes cubic spline interpolation by calculating second derivatives.
 *
 * This function computes the second derivatives necessary for cubic spline interpolation.
 * It prepares the `dd` vector, which can then be used in the cubic interpolation function.
 *
 * @tparam T The type of the coordinate, which must satisfy the Interpolatable concept.
 * @param coords A vector of coordinates for which second derivatives will be calculated.
 * @param dd The output vector that will store the calculated second derivatives.
 * @throws PathException if there are fewer than two points for interpolation.
 */
template <Interpolatable T>
void finalizeCubic(const std::vector<T>& coords, std::vector<T>& dd)
{
	const int size = static_cast<int>(coords.size());
	if (size < 2) { throw math::PathException("Not enough points for cubic interpolation"); }

	std::vector<T> tmp(size);
	dd.resize(size);

	dd.front() = 0;
	dd.back() = 0;

	for (int i = 1; i < size - 1; ++i)
	{
		const T yrd = coords[i + 1] - coords[i];
		const T yld = coords[i] - coords[i - 1];
		const RealType xrd = coords[i + 1].t - coords[i].t;
		const RealType xld = coords[i].t - coords[i - 1].t;
		const RealType iw = coords[i + 1].t - coords[i - 1].t;
		const RealType si = xld / iw;
		const T p = dd[i - 1] * si + 2.0;
		dd[i] = (si - 1.0) / p;
		tmp[i] = ((yrd / xrd - yld / xld) * 6.0 / iw - tmp[i - 1] * si) / p;
	}

	for (int i = size - 2; i >= 0; --i) { dd[i] = dd[i] * dd[i + 1] + tmp[i]; }
}
