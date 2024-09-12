// antenna_pattern.h
// Interpolated 2D arrays for gain patterns and RCS patterns
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 September 2007

#ifndef ANTENNA_PATTERN
#define ANTENNA_PATTERN

#include <string>

#include "geometry_ops.h"

namespace rs
{
	class Pattern
	{
	public:
		explicit Pattern(const std::string& filename);

		~Pattern();

		[[nodiscard]] RS_FLOAT getGain(const SVec3& angle) const;

	private:
		unsigned _size_elev{}, _size_azi{};
		RS_FLOAT** _pattern;
	};
}

#endif
