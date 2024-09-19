// interpolation_filter.h
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#ifndef INTERPOLATION_FILTER_H
#define INTERPOLATION_FILTER_H

#include <complex>
#include <functional>
#include <memory>
#include <mutex>
#include "config.h"

inline std::mutex interp_mutex;

namespace interp_filt
{
	class InterpFilter
	{
	public:
		static RS_FLOAT sinc(const RS_FLOAT x) { return x == 0.0 ? 1.0 : std::sin(x * M_PI) / (x * M_PI); }

		[[nodiscard]] RS_FLOAT kaiserWinCompute(const RS_FLOAT x) const
		{
			return x < 0 || x > _alpha * 2
				       ? 0
				       : besselI0(_beta * std::sqrt(1 - std::pow((x - _alpha) / _alpha, 2))) / _bessel_beta;
		}

		[[nodiscard]] RS_FLOAT interpFilter(const RS_FLOAT x) const { return kaiserWinCompute(x + _alpha) * sinc(x); }

		[[nodiscard]] const RS_FLOAT* getFilter(RS_FLOAT delay) const;

		static InterpFilter* getInstance();

	private:
		static RS_FLOAT besselI0(RS_FLOAT x);

		InterpFilter();

		static std::unique_ptr<InterpFilter, std::function<void(InterpFilter*)>> _instance;

		RS_FLOAT _alpha;
		RS_FLOAT _beta;
		RS_FLOAT _bessel_beta;
		int _length;
		int _table_filters;
		std::unique_ptr<RS_FLOAT[]> _filter_table;
	};
}

#endif //INTERPOLATION_FILTER_H
