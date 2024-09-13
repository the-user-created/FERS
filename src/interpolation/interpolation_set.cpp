// interpolation_set.cpp
// Implements interpolation class
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 June 2007

#include "interpolation_set.h"

#include <cmath>
#include <map>
#include <stdexcept>
#include <utility>

#include "core/logging.h"

using namespace rs;

// =====================================================================================================================
//
// INTERPOLATION SET DATA CLASS
//
// =====================================================================================================================

void InterpSetData::loadSamples(const std::vector<RS_FLOAT>& x, const std::vector<RS_FLOAT>& y)
{
	auto ix = x.begin();
	for (auto iy = y.begin(); ix != x.end() && iy != y.end(); ++ix, ++iy)
	{
		_data.insert({*ix, *iy});
	}
}

void InterpSetData::insertSample(RS_FLOAT x, RS_FLOAT y)
{
	_data.insert({x, y});
}

RS_FLOAT InterpSetData::value(const RS_FLOAT x)
{
	if (_data.empty())
	{
		throw std::logic_error("Interpolation on an empty list in InterpSet");
	}
	const auto iter = _data.lower_bound(x);
	if (iter == _data.begin())
	{
		return iter->second;
	}
	auto prev = iter;
	--prev;
	if (iter == _data.end())
	{
		return prev->second;
	}
	if (iter->first == x)
	{
		return iter->second;
	}
	const RS_FLOAT x1 = prev->first;
	const RS_FLOAT x2 = iter->first;
	const RS_FLOAT y1 = prev->second;
	const RS_FLOAT y2 = iter->second;
	return y2 * (x - x1) / (x2 - x1) + y1 * (x2 - x) / (x2 - x1);
}

RS_FLOAT InterpSetData::max() const
{
	RS_FLOAT max = 0;
	for (const auto& [fst, snd] : _data)
	{
		if (std::fabs(snd) > max)
		{
			max = std::fabs(snd);
		}
	}
	return max;
}

void InterpSetData::divide(const RS_FLOAT a)
{
	for (auto& [fst, snd] : _data)
	{
		snd /= a;
	}
}

// =====================================================================================================================
//
// INTERPOLATION SET CLASS
//
// =====================================================================================================================

InterpSet::InterpSet()
{
	_data = new InterpSetData();
}

InterpSet::~InterpSet()
{
	delete _data;
}

void InterpSet::loadSamples(const std::vector<RS_FLOAT>& x, const std::vector<RS_FLOAT>& y) const
{
	_data->loadSamples(x, y);
}

void InterpSet::insertSample(const RS_FLOAT x, const RS_FLOAT y) const
{
	_data->insertSample(x, y);
}

RS_FLOAT InterpSet::value(const RS_FLOAT x) const
{
	return _data->value(x);
}

RS_FLOAT InterpSet::max() const
{
	return _data->max();
}

void InterpSet::divide(const RS_FLOAT a) const
{
	_data->divide(a);
}
