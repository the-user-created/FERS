//rsinterp.cpp - Implements interpolation class
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//11 June 2007

#include "rsinterp.h"

#include <cmath>
#include <map>
#include <stdexcept>
#include <utility>

#include "rsdebug.h"

using std::map;
using std::vector;
using std::pair;

using namespace rs;

//
// InterpSetData Implementation
//

///Data storage class for the InterpSet class
// TODO: Why is this here and not in the header?
class rs::InterpSetData
{
public:
	///Load samples into the set
	void loadSamples(const vector<RS_FLOAT>& x, const vector<RS_FLOAT>& y);

	///Load a single sample into the set
	void insertSample(RS_FLOAT x, RS_FLOAT y);

	///Get the interpolated value at a given point
	RS_FLOAT value(RS_FLOAT x);

	/// Get the maximum value in the set
	[[nodiscard]] RS_FLOAT max() const;

	/// Divide the set by a given number
	void divide(RS_FLOAT a);

private:
	std::map<RS_FLOAT, RS_FLOAT> _data;
};


///Load samples into the set
void InterpSetData::loadSamples(const vector<RS_FLOAT>& x, const vector<RS_FLOAT>& y)
{
	auto ix = x.begin();
	for (auto iy = y.begin(); ix != x.end() && iy != y.end(); ++ix, ++iy)
	{
		_data.insert(pair(*ix, *iy));
	}
}

///Load a single sample into the set
void InterpSetData::insertSample(RS_FLOAT x, RS_FLOAT y)
{
	_data.insert(pair(x, y));
}

///Get the interpolated value for the given point
RS_FLOAT InterpSetData::value(const RS_FLOAT x)
{
	//Use linear interpolation, for now

	//If the map is empty, throw an exception and whine
	if (_data.empty())
	{
		throw std::logic_error("[BUG] Interpolation on an empty list in InterpSet");
	}
	//Get the first element with a key greater than k
	const auto iter = _data.lower_bound(x);
	//If we are at the beginning of the set, return the value
	if (iter == _data.begin())
	{
		return iter->second;
	}
	auto prev = iter;
	--prev;

	//If we are over the end, return the last value
	if (iter == _data.end())
	{
		return prev->second;
	}
		//If we hit a sample exactly, return the value
	if (iter->first == x)
	{
		return iter->second;
	}
	//Perform linear interpolation
	const RS_FLOAT x1 = prev->first;
	const RS_FLOAT x2 = iter->first;
	const RS_FLOAT y1 = prev->second;
	const RS_FLOAT y2 = iter->second;
	return y2 * (x - x1) / (x2 - x1) + y1 * (x2 - x) / (x2 - x1);
}

/// Get the maximum value in the set
RS_FLOAT InterpSetData::max() const
{
	RS_FLOAT max = 0;
	// Scan through the map, updating the maximum
	for (auto iter : _data)
	{
		if (std::fabs(iter.second) > max)
		{
			max = std::fabs(iter.second);
		}
	}
	return max;
}

/// Divide the set by a given number
void InterpSetData::divide(const RS_FLOAT a)
{
	for (auto & iter : _data)
	{
		iter.second /= a;
	}
}

//
// InterpSet Implementation
//

/// Constructor
InterpSet::InterpSet()
{
	_data = new InterpSetData();
}


/// Destructor
InterpSet::~InterpSet()
{
	delete _data;
}

/// Load a number of samples into the set
void InterpSet::loadSamples(const std::vector<RS_FLOAT>& x, const std::vector<RS_FLOAT>& y) const
{
	_data->loadSamples(x, y);
}

/// Load a single sample into the set
void InterpSet::insertSample(const RS_FLOAT x, const RS_FLOAT y) const
{
	_data->insertSample(x, y);
}

/// Get the interpolated value at the given point
RS_FLOAT InterpSet::value(const RS_FLOAT x) const
{
	return _data->value(x);
}

/// Get the maximum value in the set
RS_FLOAT InterpSet::max() const
{
	return _data->max();
}

/// Divide every sample in the set by a given number
void InterpSet::divide(const RS_FLOAT a) const
{
	_data->divide(a);
}
