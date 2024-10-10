/**
* @file interpolation_set.h
* @brief Header file for the interpolation of sets of data.
*
* This file contains the definition of classes for performing interpolation on sets of data points.
* The classes support inserting samples, retrieving interpolated values,
* finding the maximum value, and dividing the dataset by a scalar.
* It uses arithmetic types for flexibility and precision in calculations.
*
* @authors David Young, Marc Brooker
* @date 2007-06-11
*/

#pragma once

#include <map>
#include <memory>
#include <optional>

#include "config.h"

namespace interp
{
	/// Type alias for arithmetic types
	template <typename T>
	concept RealConcept = std::is_arithmetic_v<T>;

	/**
	* @class InterpSetData
	* @brief Class for managing a set of data and performing interpolation.
	*
	* The InterpSetData class manages a collection of data points and provides functionality
	* to insert samples, retrieve interpolated values at a given point, find the maximum
	* absolute value in the set, and divide all data points by a scalar.
	*/
	class InterpSetData
	{
	public:
		/**
		* @brief Inserts a sample point into the interpolation set.
		*
		* @tparam T The type of the x and y coordinates (must be an arithmetic type).
		* @param x The x-coordinate of the sample point.
		* @param y The y-coordinate of the sample point.
		*/
		template <RealConcept T>
		void insertSample(T x, T y) noexcept { _data.insert({x, y}); }

		/**
		* @brief Retrieves the interpolated value at a given point.
		*
		* @tparam T The type of the x-coordinate (must be an arithmetic type).
		* @param x The x-coordinate at which to interpolate the value.
		* @return The interpolated value at the given x-coordinate, or std::nullopt if the dataset is empty.
		*/
		template <RealConcept T>
		[[nodiscard]] std::optional<T> value(T x) const noexcept;

		/**
		* @brief Retrieves the maximum absolute value in the interpolation set.
		*
		* @return The maximum absolute y-value in the dataset, or 0.0 if the set is empty.
		*/
		[[nodiscard]] double max() const noexcept;

		/**
		* @brief Divides all y-values in the dataset by a given number.
		*
		* @tparam T The type of the divisor (must be an arithmetic type).
		* @param a The divisor to divide all y-values by.
		* @throws std::invalid_argument Thrown if the divisor is zero.
		*/
		template <RealConcept T>
		void divide(T a);

	private:
		std::map<RealType, RealType> _data; ///< The set of data points
	};

	/**
	 * @class InterpSet
	 * @brief Wrapper class for managing interpolation sets using smart pointers.
	 *
	 * The InterpSet class provides a convenient interface
	 * for working with interpolation data by managing the lifecycle of an InterpSetData object.
	 * It allows for inserting samples, retrieving interpolated values, finding the maximum value,
	 * and dividing the set by a scalar through delegation to the internal InterpSetData instance.
	 */
	class InterpSet
	{
	public:
		/**
		 * @brief Constructs a new InterpSet object.
		 *
		 * Initializes the internal InterpSetData object using a smart pointer.
		 */
		constexpr InterpSet() : _data(std::make_unique<InterpSetData>()) {}

		/// Default destructor
		constexpr ~InterpSet() = default;

		/**
		 * @brief Inserts a sample point into the interpolation set.
		 *
		 * @tparam T The type of the x and y coordinates (must be an arithmetic type).
		 * @param x The x-coordinate of the sample point.
		 * @param y The y-coordinate of the sample point.
		 */
		template <RealConcept T>
		void insertSample(T x, T y) const noexcept { _data->insertSample(x, y); }

		/**
		 * @brief Retrieves the interpolated value at a given point.
		 *
		 * @tparam T The type of the x-coordinate (must be an arithmetic type).
		 * @param x The x-coordinate at which to interpolate the value.
		 * @return The interpolated value at the given x-coordinate, or std::nullopt if the dataset is empty.
		 */
		template <RealConcept T>
		[[nodiscard]] std::optional<T> getValueAt(T x) const noexcept { return _data->value(x); }

		/**
		 * @brief Retrieves the maximum absolute value in the interpolation set.
		 *
		 * @return The maximum absolute y-value in the dataset, or 0.0 if the set is empty.
		 */
		[[nodiscard]] double getMax() const noexcept { return _data->max(); }

		/**
		 * @brief Divides all y-values in the dataset by a given number.
		 *
		 * @tparam T The type of the divisor (must be an arithmetic type).
		 * @param a The divisor to divide all y-values by.
		 * @throws std::invalid_argument Thrown if the divisor is zero.
		 */
		template <RealConcept T>
		void divide(T a) const { _data->divide(a); }

	private:
		std::unique_ptr<InterpSetData> _data; ///< The internal InterpSetData object managed by a smart pointer
	};
}
