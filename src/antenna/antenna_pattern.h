// antenna_pattern.h
// Interpolated 2D arrays for gain patterns and RCS patterns
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 September 2007

#ifndef ANTENNA_PATTERN
#define ANTENNA_PATTERN

#include <string>
#include <vector>

#include "config.h"

namespace serial
{
	std::vector<std::vector<RealType>> readPattern(const std::string& name, const std::string& datasetName, unsigned& aziSize, unsigned& elevSize);
}

namespace math {
	class SVec3;
}

namespace antenna
{
	class Pattern
	{
	public:
		explicit Pattern(const std::string& filename)
		{
			_pattern = serial::readPattern(filename, "antenna", _size_azi, _size_elev);
		}

		~Pattern() = default;

		[[nodiscard]] RealType getGain(const math::SVec3& angle) const;

	private:
		unsigned _size_elev{}, _size_azi{};
		std::vector<std::vector<RealType>> _pattern;
	};
}

#endif
