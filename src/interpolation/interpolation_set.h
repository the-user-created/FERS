// interpolation_set.cpp
// Classes for interpolation of sets of data
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 June 2007

#ifndef INTERPOLATION_SET_H
#define INTERPOLATION_SET_H

#include <map>
#include <memory>
#include <vector>

#include "config.h"

namespace interp
{
	class InterpSetData
	{
	public:
		///Load samples into the set
		void loadSamples(const std::vector<RS_FLOAT>& x, const std::vector<RS_FLOAT>& y);

		///Load a single sample into the set
		void insertSample(RS_FLOAT x, RS_FLOAT y) { _data.insert({x, y}); }

		///Get the interpolated value at a given point
		RS_FLOAT value(RS_FLOAT x);

		/// Get the maximum value in the set
		[[nodiscard]] RS_FLOAT max() const;

		/// Divide the set by a given number
		void divide(RS_FLOAT a);

	private:
		std::map<RS_FLOAT, RS_FLOAT> _data;
	};

	class InterpSet
	{
	public:
		InterpSet() { _data = std::make_unique<InterpSetData>(); }

		~InterpSet() = default;

		// Note: This function is not used in the codebase
		void loadSamples(const std::vector<RS_FLOAT>& x, const std::vector<RS_FLOAT>& y) const
		{
			_data->loadSamples(x, y);
		}

		void insertSample(const RS_FLOAT x, const RS_FLOAT y) const { _data->insertSample(x, y); }

		[[nodiscard]] RS_FLOAT getValueAt(const RS_FLOAT x) const { return _data->value(x); }

		[[nodiscard]] RS_FLOAT getMax() const { return _data->max(); }

		void divide(const RS_FLOAT a) const { _data->divide(a); }

	private:
		std::unique_ptr<InterpSetData> _data;
	};
}

#endif
