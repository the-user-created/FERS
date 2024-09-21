// interpolation_set.cpp
// Implements interpolation class
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 June 2007

#include "interpolation_set.h"

#include <algorithm>
#include <ranges>

namespace interp
{
	// =================================================================================================================
	//
	// INTERPOLATION SET DATA CLASS
	//
	// =================================================================================================================

	template <RealConcept T>
	void InterpSetData::loadSamples(const std::vector<T>& x, const std::vector<T>& y)
	{
		if (x.size() != y.size()) { throw std::invalid_argument("X and Y vectors must have the same size"); }

		for (size_t i = 0; i < x.size(); ++i) { _data.insert({static_cast<double>(x[i]), static_cast<double>(y[i])}); }
	}

	template <RealConcept T>
	std::optional<T> InterpSetData::value(T x) const
	{
		if (_data.empty()) { return std::nullopt; }

		const auto iter = _data.lower_bound(static_cast<double>(x));

		if (iter == _data.begin()) { return static_cast<T>(iter->second); }
		if (iter == _data.end())
		{
			const auto prev = std::prev(iter);
			return static_cast<T>(prev->second);
		}
		if (iter->first == static_cast<double>(x)) { return static_cast<T>(iter->second); }

		auto prev = std::prev(iter);
		const auto [x1, y1] = *prev;
		const auto [x2, y2] = *iter;

		return static_cast<T>(y2 * (x - x1) / (x2 - x1) + y1 * (x2 - x) / (x2 - x1));
	}

	// Returns the maximum absolute value as a double
	double InterpSetData::max() const
	{
		auto values = _data | std::views::values;

		const auto max_element = std::ranges::max_element(values, [](const double a, const double b)
		{
			return std::abs(a) < std::abs(b);
		});

		return max_element != values.end() ? std::abs(*max_element) : 0.0;
	}

	template <RealConcept T>
	void InterpSetData::divide(T a)
	{
		if (a == 0) { throw std::invalid_argument("Division by zero is not allowed."); }

		for (auto& value : _data | std::views::values) { value /= static_cast<double>(a); }
	}

	// Explicit instantiations for double and float (or any other type you want)
	template void InterpSetData::loadSamples<double>(const std::vector<double>&, const std::vector<double>&);

	template std::optional<double> InterpSetData::value<double>(double) const;

	template void InterpSetData::divide<double>(double);

	template void InterpSetData::loadSamples<float>(const std::vector<float>&, const std::vector<float>&);

	template std::optional<float> InterpSetData::value<float>(float) const;

	template void InterpSetData::divide<float>(float);
}
