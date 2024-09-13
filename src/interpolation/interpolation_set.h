// interpolation_set.cpp
// Classes for interpolation of sets of data
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 June 2007

#ifndef INTERPOLATION_SET_H
#define INTERPOLATION_SET_H

#include <map>
#include <vector>

#include "config.h"

namespace rs
{
	class InterpSetData
	{
	public:
		///Load samples into the set
		void loadSamples(const std::vector<RS_FLOAT>& x, const std::vector<RS_FLOAT>& y);

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

	class InterpSet
	{
	public:
		InterpSet();

		~InterpSet();

		void loadSamples(const std::vector<RS_FLOAT>& x, const std::vector<RS_FLOAT>& y) const;

		void insertSample(RS_FLOAT x, RS_FLOAT y) const;

		[[nodiscard]] RS_FLOAT value(RS_FLOAT x) const;

		[[nodiscard]] RS_FLOAT max() const;

		void divide(RS_FLOAT a) const;

	private:
		InterpSetData* _data;
	};
}

#endif
