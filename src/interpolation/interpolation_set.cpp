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

	void InterpSetData::loadSamples(const std::vector<RS_FLOAT>& x, const std::vector<RS_FLOAT>& y)
	{
		auto ix = x.begin();
		for (auto iy = y.begin(); ix != x.end() && iy != y.end(); ++ix, ++iy) { _data.insert({*ix, *iy}); }
	}

	RS_FLOAT InterpSetData::value(const RS_FLOAT x)
	{
		if (_data.empty()) { throw std::logic_error("Interpolation on an empty list in InterpSet"); }
		const auto iter = _data.lower_bound(x);
		if (iter == _data.begin()) { return iter->second; }
		auto prev = iter;
		--prev;
		if (iter == _data.end()) { return prev->second; }
		if (iter->first == x) { return iter->second; }
		const RS_FLOAT x1 = prev->first;
		const RS_FLOAT x2 = iter->first;
		const RS_FLOAT y1 = prev->second;
		const RS_FLOAT y2 = iter->second;
		return y2 * (x - x1) / (x2 - x1) + y1 * (x2 - x) / (x2 - x1);
	}

	RS_FLOAT InterpSetData::max() const
	{
		// Use views::values to access the second elements in the map
		auto values = _data | std::views::values;

		// Find the maximum of the absolute values
		const auto max_element = std::ranges::max_element(values, [](const RS_FLOAT a, const RS_FLOAT b)
		{
			return std::fabs(a) < std::fabs(b);
		});

		return max_element != values.end() ? std::fabs(*max_element) : 0.0f;
	}

	void InterpSetData::divide(const RS_FLOAT a)
	{
		// Use views::values to directly modify the second elements in the map
		auto values = _data | std::views::values;
		std::ranges::for_each(values, [a](RS_FLOAT& val) { val /= a; });
	}
}