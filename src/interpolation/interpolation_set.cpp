// interpolation_set.cpp
// Implements interpolation class
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 June 2007

#include "interpolation_set.h"

#include <algorithm>
#include <cmath>
#include <ranges>

namespace interp
{
	// =================================================================================================================
	//
	// INTERPOLATION SET DATA CLASS
	//
	// =================================================================================================================

	void InterpSetData::loadSamples(const std::vector<RealType>& x, const std::vector<RealType>& y)
	{
		auto ix = x.begin();
		for (auto iy = y.begin(); ix != x.end() && iy != y.end(); ++ix, ++iy) { _data.insert({*ix, *iy}); }
	}

	RealType InterpSetData::value(const RealType x)
	{
		if (_data.empty()) { throw std::logic_error("Interpolation on an empty list in InterpSet"); }
		const auto iter = _data.lower_bound(x);
		if (iter == _data.begin()) { return iter->second; }
		auto prev = iter;
		--prev;
		if (iter == _data.end()) { return prev->second; }
		if (iter->first == x) { return iter->second; }
		const RealType x1 = prev->first;
		const RealType x2 = iter->first;
		const RealType y1 = prev->second;
		const RealType y2 = iter->second;
		return y2 * (x - x1) / (x2 - x1) + y1 * (x2 - x) / (x2 - x1);
	}

	RealType InterpSetData::max() const
	{
		// Use views::values to access the second elements in the map
		auto values = _data | std::views::values;

		// Find the maximum of the absolute values
		const auto max_element = std::ranges::max_element(values, [](const RealType a, const RealType b)
		{
			return std::fabs(a) < std::fabs(b);
		});

		return max_element != values.end() ? std::fabs(*max_element) : 0.0f;
	}

	void InterpSetData::divide(const RealType a)
	{
		// Use views::values to directly modify the second elements in the map
		auto values = _data | std::views::values;
		std::ranges::for_each(values, [a](RealType& val) { val /= a; });
	}
}