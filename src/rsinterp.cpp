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
	void loadSamples(const vector<rsFloat>& x, const vector<rsFloat>& y);

	///Load a single sample into the set
	void insertSample(rsFloat x, rsFloat y);

	///Get the interpolated value at a given point
	rsFloat value(rsFloat x);

	/// Get the maximum value in the set
	rsFloat max() const;

	/// Divide the set by a given number
	void divide(rsFloat a);

private:
	std::map<rsFloat, rsFloat> _data;
};


///Load samples into the set
void InterpSetData::loadSamples(const vector<rsFloat>& x, const vector<rsFloat>& y)
{
	vector<rsFloat>::const_iterator ix = x.begin();
	for (vector<rsFloat>::const_iterator iy = y.begin(); (ix != x.end()) && (iy != y.end()); ++ix, ++iy)
	{
		_data.insert(pair<rsFloat, rsFloat>(*ix, *iy));
	}
}

///Load a single sample into the set
void InterpSetData::insertSample(rsFloat x, rsFloat y)
{
	_data.insert(pair<rsFloat, rsFloat>(x, y));
}

///Get the interpolated value for the given point
rsFloat InterpSetData::value(const rsFloat x)
{
	//Use linear interpolation, for now

	//If the map is empty, throw an exception and whine
	if (_data.empty())
	{
		throw std::logic_error("[BUG] Interpolation on an empty list in InterpSet");
	}
	//Get the first element with a key greater than k
	const map<rsFloat, rsFloat>::const_iterator iter = _data.lower_bound(x);
	//If we are at the beginning of the set, return the value
	if (iter == _data.begin())
	{
		return (*iter).second;
	}
	map<rsFloat, rsFloat>::const_iterator prev = iter;
	--prev;

	//If we are over the end, return the last value
	if (iter == _data.end())
	{
		return (*(prev)).second;
	}
		//If we hit a sample exactly, return the value
	else if ((*iter).first == x)
	{
		return (*iter).second;
	}
	//Perform linear interpolation
	else
	{
		const rsFloat x1 = (*prev).first;
		const rsFloat x2 = (*iter).first;
		const rsFloat y1 = (*prev).second;
		const rsFloat y2 = (*iter).second;
		return y2 * (x - x1) / (x2 - x1) + y1 * (x2 - x) / (x2 - x1);
	}
}

/// Get the maximum value in the set
rsFloat InterpSetData::max() const
{
	rsFloat max = 0;
	// Scan through the map, updating the maximum
	for (map<rsFloat, rsFloat>::const_iterator iter = _data.begin(); iter != _data.end(); ++iter)
	{
		if (std::fabs((*iter).second) > max)
		{
			max = std::fabs((*iter).second);
		}
	}
	return max;
}

/// Divide the set by a given number
void InterpSetData::divide(const rsFloat a)
{
	for (map<rsFloat, rsFloat>::iterator iter = _data.begin(); iter != _data.end(); ++iter)
	{
		(*iter).second /= a;
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
void InterpSet::loadSamples(const std::vector<rsFloat>& x, const std::vector<rsFloat>& y) const
{
	_data->loadSamples(x, y);
}

/// Load a single sample into the set
void InterpSet::insertSample(const rsFloat x, const rsFloat y) const
{
	_data->insertSample(x, y);
}

/// Get the interpolated value at the given point
rsFloat InterpSet::value(const rsFloat x) const
{
	return _data->value(x);
}

/// Get the maximum value in the set
rsFloat InterpSet::max() const
{
	return _data->max();
}

/// Divide every sample in the set by a given number
void InterpSet::divide(const rsFloat a) const
{
	_data->divide(a);
}
