/**
* @file geometry_ops.h
* @brief Classes and operations for 3D geometry.
*
* This file defines the Vec3, SVec3, and Matrix3 classes for handling 3D vector and matrix operations.
* It includes various mathematical operations such as dot products, cross products, and operator overloads
* for vector and matrix manipulations.
* The code provides tools to work with rectangular and spherical  coordinate systems.
*
* @authors David Young, Marc Brooker
* @date 2006-05-26
*/

#pragma once

#include <array>
#include <cmath>

#include "config.h"

namespace math
{
	class Vec3;

	/**
	* @class Matrix3
	* @brief A class representing a 3x3 matrix.
	*
	* This class is used for handling 3x3 matrices with basic functionalities such as retrieving the data array.
	*/
	class Matrix3
	{
	public:
		std::array<RealType, 9> elements{}; ///< The 3x3 matrix elements

		/**
		* @brief Default constructor for Matrix3.
		*/
		Matrix3() = default;

		/**
		* @brief Default destructor for Matrix3.
		*/
		~Matrix3() = default;

		/**
		* @brief Get the matrix data as a constant pointer.
		*
		* This function returns the internal array of the matrix in a read-only manner.
		* @return A constant pointer to the matrix elements.
		*/
		[[nodiscard]] const RealType* getData() const noexcept { return elements.data(); }

		/**
		* @brief Get the matrix data as a modifiable pointer.
		*
		* This function returns the internal array of the matrix which can be modified.
		* @return A pointer to the matrix elements.
		*/
		RealType* getData() noexcept { return elements.data(); }
	};

	/**
	* @class SVec3
	* @brief A class representing a vector in spherical coordinates.
	*
	* This class is used to represent 3D vectors using spherical coordinates with properties for length, azimuth, and elevation.
	*/
	class SVec3
	{
	public:
		RealType length{}; ///< The length of the vector
		RealType azimuth{}; ///< The azimuth angle of the vector
		RealType elevation{}; ///< The elevation angle of the vector

		/**
		* @brief Default constructor for SVec3.
		*/
		constexpr SVec3() noexcept = default;

		/**
		* @brief Parameterized constructor for SVec3.
		*
		* Initializes the spherical vector with length, azimuth, and elevation.
		* @param length The length or magnitude of the vector.
		* @param azimuth The azimuthal angle of the vector.
		* @param elevation The elevation angle of the vector.
		*/
		constexpr SVec3(const RealType length, const RealType azimuth, const RealType elevation) noexcept
			: length(length), azimuth(azimuth), elevation(elevation) {}

		/**
		* @brief Constructs a spherical vector from a rectangular Vec3.
		*
		* Converts the given rectangular vector into spherical coordinates.
		* @param vec A rectangular vector (Vec3) to convert.
		*/
		explicit SVec3(const Vec3& vec) noexcept;

		/**
		* @brief Default destructor for SVec3.
		*/
		~SVec3() = default;

		/**
		* @brief Scalar multiplication assignment for SVec3.
		*
		* Multiplies the length of the spherical vector by a scalar value.
		* @param b The scalar value.
		* @return A reference to the updated SVec3.
		*/
		SVec3& operator*=(RealType b) noexcept;

		/**
		* @brief Scalar division assignment for SVec3.
		*
		* Divides the length of the spherical vector by a scalar value.
		* @param b The scalar value.
		* @return A reference to the updated SVec3.
		*/
		SVec3& operator/=(RealType b) noexcept;
	};

	/**
	* @class Vec3
	* @brief A class representing a vector in rectangular coordinates.
	*
	* This class is used to represent 3D vectors with x, y, and z components and includes operations
	* for vector addition, subtraction, scalar multiplication, and matrix transformation.
	*/
	class Vec3
	{
	public:
		RealType x{}; ///< The x component of the vector
		RealType y{}; ///< The y component of the vector
		RealType z{}; ///< The z component of the vector

		/**
		* @brief Default constructor for Vec3.
		*/
		constexpr Vec3() noexcept = default;

		/**
		* @brief Parameterized constructor for Vec3.
		*
		* Initializes the vector with the given x, y, and z components.
		* @param x The x component.
		* @param y The y component.
		* @param z The z component.
		*/
		constexpr Vec3(const RealType x, const RealType y, const RealType z) noexcept : x(x), y(y), z(z) {}

		/**
		* @brief Constructs a rectangular vector from a spherical SVec3.
		*
		* Converts the given spherical vector into rectangular coordinates.
		* @param svec A spherical vector (SVec3) to convert.
		*/
		explicit Vec3(const SVec3& svec) noexcept;

		/**
		* @brief Addition assignment operator for Vec3.
		*
		* Adds the components of another Vec3 to this vector.
		* @param b The vector to add.
		* @return A reference to the updated Vec3.
		*/
		Vec3& operator+=(const Vec3& b) noexcept;

		/**
		* @brief Subtraction assignment operator for Vec3.
		*
		* Subtracts the components of another Vec3 from this vector.
		* @param b The vector to subtract.
		* @return A reference to the updated Vec3.
		*/
		Vec3& operator-=(const Vec3& b) noexcept;

		/**
		* @brief Multiplication assignment operator for Vec3.
		*
		* Multiplies the components of this vector by the components of another Vec3.
		* @param b The vector to multiply by.
		* @return A reference to the updated Vec3.
		*/
		Vec3& operator*=(const Vec3& b) noexcept;

		/**
		* @brief Matrix multiplication assignment for Vec3.
		*
		* Multiplies this vector by a 3x3 matrix, transforming it.
		* @param m The matrix to multiply by.
		* @return A reference to the updated Vec3.
		*/
		Vec3& operator*=(const Matrix3& m) noexcept;

		/**
		* @brief Scalar multiplication assignment for Vec3.
		*
		* Multiplies the components of this vector by a scalar value.
		* @param b The scalar value.
		* @return A reference to the updated Vec3.
		*/
		Vec3& operator*=(RealType b) noexcept;

		/**
		* @brief Scalar division assignment for Vec3.
		*
		* Divides the components of this vector by a scalar value.
		* @param b The scalar value.
		* @return A reference to the updated Vec3.
		*/
		Vec3& operator/=(RealType b) noexcept;

		/**
		 * @brief Addition operator for Vec3.
		 *
		 * Adds a scalar value to each component of the vector.
		 * @param value The scalar value to add.
		 * @return The resulting vector.
		 */
		Vec3 operator+(const RealType value) const { return {x + value, y + value, z + value}; }

		/**
		* @brief Calculates the length (magnitude) of the vector.
		*
		* Computes the Euclidean length of the vector.
		* @return The length of the vector.
		*/
		[[nodiscard]] RealType length() const noexcept { return std::sqrt(x * x + y * y + z * z); }
	};

	/**
	* @brief Computes the dot product of two Vec3 vectors.
	*
	* This function returns the dot product of the two input vectors.
	* @param a The first vector.
	* @param b The second vector.
	* @return The dot product of the vectors.
	*/
	inline RealType dotProduct(const Vec3& a, const Vec3& b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z; }

	// Cross product
	// Note: This function is not used in the codebase
	/*inline Vec3 crossProduct(const Vec3& a, const Vec3& b) noexcept
	{
		return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
	}*/

	/// Multiplies two Vec3 vectors component-wise
	inline Vec3 operator*(const Vec3& a, const Vec3& b) noexcept { return {a.x * b.x, a.y * b.y, a.z * b.z}; }

	/// Adds two Vec3 vectors component-wise
	inline Vec3 operator+(const Vec3& a, const Vec3& b) noexcept { return {a.x + b.x, a.y + b.y, a.z + b.z}; }

	/// Subtracts two Vec3 vectors component-wise
	inline Vec3 operator-(const Vec3& a, const Vec3& b) noexcept { return {a.x - b.x, a.y - b.y, a.z - b.z}; }

	/// Divides two Vec3 vectors component-wise
	inline Vec3 operator/(const Vec3& a, const Vec3& b) { return {a.x / b.x, a.y / b.y, a.z / b.z}; }

	/// Multiplies a Vec3 vector by a scalar value
	inline Vec3 operator*(const Vec3& a, const RealType b) noexcept { return {a.x * b, a.y * b, a.z * b}; }

	/// Divides a Vec3 vector by a scalar value
	inline Vec3 operator/(const Vec3& a, const RealType b) noexcept { return {a.x / b, a.y / b, a.z / b}; }

	/// Divides a scalar value by a Vec3 vector
	inline Vec3 operator/(const RealType a, const Vec3& b) noexcept { return {a / b.x, a / b.y, a / b.z}; }

	/// Adds two SVec3 vectors
	SVec3 operator+(const SVec3& a, const SVec3& b) noexcept;

	/// Subtracts two SVec3 vectors
	SVec3 operator-(const SVec3& a, const SVec3& b) noexcept;
}
