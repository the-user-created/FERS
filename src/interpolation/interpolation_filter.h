// interpolation_filter.h
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef INTERPOLATION_FILTER_H
#define INTERPOLATION_FILTER_H

#include <cmath>
#include <vector>

#include "config.h"

namespace interp
{
	class InterpFilter
	{
	public:
		static RealType sinc(const RealType x) { return x == 0.0 ? 1.0 : std::sin(x * M_PI) / (x * M_PI); }

		[[nodiscard]] RealType kaiserWinCompute(const RealType x) const
		{
			return x < 0 || x > _alpha * 2
				       ? 0
				       : besselI0(_beta * std::sqrt(1 - std::pow((x - _alpha) / _alpha, 2))) / _bessel_beta;
		}

		[[nodiscard]] RealType interpFilter(const RealType x) const { return kaiserWinCompute(x + _alpha) * sinc(x); }

		[[nodiscard]] const RealType* getFilter(RealType delay) const;

		static InterpFilter* getInstance();

	private:
		static RealType besselI0(RealType x);

		InterpFilter();

		RealType _alpha;
		RealType _beta = 5; // Beta sets the window shape
		RealType _bessel_beta;
		int _length;
		int _table_filters;
		std::vector<RealType> _filter_table;
	};
}

#endif //INTERPOLATION_FILTER_H
