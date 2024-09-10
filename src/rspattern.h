//rspattern.h
// Interpolated 2D arrays for gain patterns and RCS patterns
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//11 September 2007

#ifndef RS_PATTERN_H
#define RS_PATTERN_H

#include <string>

#include "rsgeometry.h"

namespace rs
{
	///Class to manage and interpolate a 2D pattern
	class Pattern
	{
	public:
		/// Constructor
		explicit Pattern(const std::string& filename);

		/// Destructor
		~Pattern();

		/// Get the gain at the given angle
		RS_FLOAT getGain(const SVec3& angle) const;

	private:
		unsigned int _size_elev, _size_azi; //!< Number of samples in elevation and azimuth
		RS_FLOAT** _pattern; //!< 2D Array to store the gain pattern
	};
}

#endif
