// interpolation_filter.h
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#pragma once

#include <cmath>
#include <span>
#include <vector>

#include "config.h"

namespace interp
{
	class InterpFilter
	{
	public:
		// constexpr for compile-time evaluation
		static constexpr RealType sinc(const RealType x) noexcept
		{
			return x == 0.0 ? 1.0 : std::sin(x * PI) / (x * PI);
		}

		[[nodiscard]] RealType kaiserWinCompute(RealType x) const noexcept;

		[[nodiscard]] RealType interpFilter(RealType x) const noexcept;

		[[nodiscard]] std::span<const RealType> getFilter(RealType delay) const;

		static InterpFilter& getInstance();

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
