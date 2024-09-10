//rsinterp.cpp - Classes for interpolation of sets of data
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//11 June 2007

#ifndef RS_INTERP_H
#define RS_INTERP_H

#include <vector>

#include "config.h"

namespace rs
{
	class InterpSetData;

	/// This class interpolates y, given x based on a number of samples of y
	/// The samples do not need to be equally spaced in x
	class InterpSet
	{
		//InterpSet is just a thin wrapper around InterpSetData, the class which implements interpolation
		//this was done to allow interpolation to be performed with different algorithms, as needed
	public:
		/// Constructor
		InterpSet();

		/// Destructor
		~InterpSet();

		/// Load a number of samples into the set
		void loadSamples(const std::vector<RS_FLOAT>& x, const std::vector<RS_FLOAT>& y) const;

		/// Load a single sample into the set
		void insertSample(RS_FLOAT x, RS_FLOAT y) const;

		/// Get the interpolated value at the given point
		RS_FLOAT value(RS_FLOAT x) const;

		/// Get the maximum value in the set
		RS_FLOAT max() const;

		/// Divide every sample in the set by a given number
		void divide(RS_FLOAT a) const;

	private:
		InterpSetData* _data; //<! The data is hidden behind a pointer to the implementation
	};
}

#endif
