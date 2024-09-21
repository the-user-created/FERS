// interpolation_set.cpp
// Classes for interpolation of sets of data
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 June 2007

#ifndef INTERPOLATION_SET_H
#define INTERPOLATION_SET_H

#include <cmath>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "config.h"

namespace interp
{
	// Concept to constrain types to arithmetic types (e.g., float, double)
	template <typename T>
	concept RealConcept = std::is_arithmetic_v<T>;

	class InterpSetData
	{
	public:
		/// Load samples into the set
		template <RealConcept T>
		void loadSamples(const std::vector<T>& x, const std::vector<T>& y);

		/// Load a single sample into the set
		template <RealConcept T>
		void insertSample(T x, T y) { _data.insert({x, y}); }

		/// Get the interpolated value at a given point
		template <RealConcept T>
		[[nodiscard]] std::optional<T> value(T x) const;

		/// Get the maximum value in the set (always returning a double)
		[[nodiscard]] double max() const;

		/// Divide the set by a given number
		template <RealConcept T>
		void divide(T a);

	private:
		std::map<RealType, RealType> _data;
	};

	class InterpSet
	{
	public:
		constexpr InterpSet() : _data(std::make_unique<InterpSetData>()) {}

		// Rule of five compliance handled implicitly by smart pointers
		constexpr ~InterpSet() = default;

		template <RealConcept T>
		void loadSamples(const std::vector<T>& x, const std::vector<T>& y) const { _data->loadSamples(x, y); }

		template <RealConcept T>
		void insertSample(T x, T y) const { _data->insertSample(x, y); }

		template <RealConcept T>
		[[nodiscard]] std::optional<T> getValueAt(T x) const { return _data->value(x); }

		[[nodiscard]] double getMax() const { return _data->max(); }

		template <RealConcept T>
		void divide(T a) const { _data->divide(a); }

	private:
		std::unique_ptr<InterpSetData> _data;
	};
}

#endif
