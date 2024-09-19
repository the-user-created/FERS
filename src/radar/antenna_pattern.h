// antenna_pattern.h
// Interpolated 2D arrays for gain patterns and RCS patterns
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 September 2007

#ifndef ANTENNA_PATTERN
#define ANTENNA_PATTERN

#include <string>
#include <vector>

#include "math_utils/geometry_ops.h"
#include "serialization/hdf5_export.h"

namespace rs
{
	class Pattern
	{
	public:
		explicit Pattern(const std::string& filename)
		{
			_pattern = hdf5_export::readPattern(filename, "antenna", _size_azi, _size_elev);
		}

		~Pattern() = default;

		[[nodiscard]] RS_FLOAT getGain(const SVec3& angle) const;

	private:
		unsigned _size_elev{}, _size_azi{};
		std::vector<std::vector<RS_FLOAT>> _pattern;
	};
}

#endif
