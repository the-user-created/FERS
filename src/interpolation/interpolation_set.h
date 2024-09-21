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
		void loadSamples(const std::vector<RealType>& x, const std::vector<RealType>& y);

		///Load a single sample into the set
		void insertSample(RealType x, RealType y) { _data.insert({x, y}); }

		///Get the interpolated value at a given point
		RealType value(RealType x);

		/// Get the maximum value in the set
		[[nodiscard]] RealType max() const;

		/// Divide the set by a given number
		void divide(RealType a);

	private:
		std::map<RealType, RealType> _data;
	};

	class InterpSet
	{
	public:
		InterpSet() { _data = std::make_unique<InterpSetData>(); }

		~InterpSet() = default;

		// Note: This function is not used in the codebase
		void loadSamples(const std::vector<RealType>& x, const std::vector<RealType>& y) const
		{
			_data->loadSamples(x, y);
		}

		void insertSample(const RealType x, const RealType y) const { _data->insertSample(x, y); }

		[[nodiscard]] RealType getValueAt(const RealType x) const { return _data->value(x); }

		[[nodiscard]] RealType getMax() const { return _data->max(); }

		void divide(const RealType a) const { _data->divide(a); }

	private:
		std::unique_ptr<InterpSetData> _data;
	};
}

#endif
