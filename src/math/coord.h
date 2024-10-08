/**
 * @file coord.h
 * @brief Coordinate and rotation structure operations.
 *
 * This file defines the `Coord` and `RotationCoord`
 * structures along with various operator overloads for performing mathematical operations on them.
 * These structures are used to represent position coordinates and rotation angles,
 * respectively, in a 3D space, along with time (t).
 *
 * @authors David Young, Marc Brooker
 * @date 2024-09-13
 */

#pragma once

#include "geometry_ops.h"

namespace math
{
	/**
	 * @class Coord
	 * @brief Represents a position in 3D space with an associated time.
	 *
	 * The `Coord` structure holds a 3D position (`pos`) using the `Vec3` type, and a time component (`t`).
	 * This class also provides various operator overloads for mathematical operations such as addition,
	 * multiplication, and division.
	 */
	struct Coord
	{
		Vec3 pos; ///< 3D position
		RealType t; ///< Time

		/**
		 * @brief Comparison operator based on the time component.
		 *
		 * @param b The other `Coord` to compare.
		 * @return True if the current object's time is less than the other.
		 */
		bool operator<(const Coord& b) const noexcept { return t < b.t; }

		/**
		 * @brief Assignment operator to set the time and position.
		 *
		 * @param a Scalar value to assign to the position and time.
		 * @return Reference to the modified `Coord` object.
		 */
		Coord& operator=(RealType a) noexcept
		{
			t = a;
			pos = {a, a, a};
			return *this;
		}
	};

	/// Multiplies two `Coord` objects' positions and copies the time.
	inline Coord operator*(const Coord& a, const Coord& b) noexcept { return {a.pos * b.pos, a.t}; }
	/// Adds two `Coord` objects' positions and copies the time.
	inline Coord operator+(const Coord& a, const Coord& b) noexcept { return {a.pos + b.pos, a.t}; }
	/// Subtracts two `Coord` objects' positions and copies the time.
	inline Coord operator-(const Coord& a, const Coord& b) noexcept { return {a.pos - b.pos, a.t}; }
	/// Divides two `Coord` objects' positions and copies the time.
	inline Coord operator/(const Coord& a, const Coord& b) noexcept { return {a.pos / b.pos, a.t}; }
	/// Adds a scalar to a `Coord`'s position while copying the time.
	inline Coord operator+(const Coord& a, const RealType b) noexcept { return {a.pos + b, a.t}; }
	/// Multiplies a `Coord`'s position by a scalar while copying the time.
	inline Coord operator*(const Coord& a, const RealType b) noexcept { return {a.pos * b, a.t}; }
	/// Divides a scalar by a `Coord`'s position and copies the time.
	inline Coord operator/(const RealType a, const Coord& b) noexcept { return {a / b.pos, b.t}; }
	/// Divides a `Coord`'s position by a scalar while copying the time.
	inline Coord operator/(const Coord& b, const RealType a) noexcept { return {b.pos / a, b.t}; }

	/**
	 * @class RotationCoord
	 * @brief Represents a rotation in terms of azimuth, elevation, and time.
	 *
	 * The `RotationCoord` structure stores rotational information,
	 * with azimuth and elevation angles, as well as an associated time (`t`).
	 * Various operator overloads are provided for manipulating rotation data.
	 */
	struct RotationCoord
	{
		RealType azimuth{}; ///< Azimuth angle
		RealType elevation{}; ///< Elevation angle
		RealType t{}; ///< Time

		/**
		 * @brief Comparison operator based on the time component.
		 *
		 * @param b The other `RotationCoord` to compare.
		 * @return True if the current object's time is less than the other.
		 */
		bool operator<(const RotationCoord& b) const noexcept { return t < b.t; }

		/**
		 * @brief Assignment operator to set azimuth, elevation, and time.
		 *
		 * @param a Scalar value to assign to all components.
		 * @return Reference to the modified `RotationCoord` object.
		 */
		RotationCoord& operator=(const RealType a) noexcept
		{
			azimuth = elevation = t = a;
			return *this;
		}

		/**
		 * @brief Constructor to initialize azimuth, elevation, and time.
		 *
		 * @param a Scalar value to initialize azimuth, elevation, and time.
		 */
		constexpr explicit RotationCoord(const RealType a = 0) noexcept : azimuth(a), elevation(a), t(a) {}

		/**
		* @brief Constructor to initialize azimuth, elevation, and time.
		*
		* @param az Azimuth angle.
		* @param el Elevation angle.
		* @param time Time component.
		*/
		RotationCoord(const RealType az, const RealType el, const RealType time) noexcept
			: azimuth(az), elevation(el), t(time) {}
	};

	/// Multiplies two `RotationCoord` objects' components and copies the time.
	inline RotationCoord operator*(const RotationCoord& a, const RotationCoord& b) noexcept
	{
		return {a.azimuth * b.azimuth, a.elevation * b.elevation, a.t};
	}

	/// Adds two `RotationCoord` objects' components and copies the time.
	inline RotationCoord operator+(const RotationCoord& a, const RotationCoord& b) noexcept
	{
		return {a.azimuth + b.azimuth, a.elevation + b.elevation, a.t};
	}

	/// Subtracts two `RotationCoord` objects' components and copies the time.
	inline RotationCoord operator-(const RotationCoord& a, const RotationCoord& b) noexcept
	{
		return {a.azimuth - b.azimuth, a.elevation - b.elevation, a.t};
	}

	/// Divides two `RotationCoord` objects' components and copies the time.
	inline RotationCoord operator/(const RotationCoord& a, const RotationCoord& b) noexcept
	{
		return {a.azimuth / b.azimuth, a.elevation / b.elevation, a.t};
	}

	/// Adds a scalar to a `RotationCoord`'s components while copying the time.
	inline RotationCoord operator+(const RotationCoord& a, const RealType b) noexcept
	{
		return {a.azimuth + b, a.elevation + b, a.t};
	}

	/// Multiplies a `RotationCoord`'s components by a scalar while copying the time.
	inline RotationCoord operator*(const RotationCoord& a, const RealType b) noexcept
	{
		return {a.azimuth * b, a.elevation * b, a.t};
	}

	/// Divides a scalar by a `RotationCoord`'s components and copies the time.
	inline RotationCoord operator/(const RealType a, const RotationCoord& b) noexcept
	{
		return {a / b.azimuth, a / b.elevation, b.t};
	}

	/// Divides a `RotationCoord`'s components by a scalar while copying the time.
	inline RotationCoord operator/(const RotationCoord& b, const RealType a) noexcept
	{
		return {b.azimuth / a, b.elevation / a, b.t};
	}
}
