// interpolation_filter.h
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#pragma once

#include <cmath>
#include <expected>
#include <span>
#include <vector>

#include "config.h"

namespace interp
{
	class InterpFilter
	{
	public:
		static constexpr RealType sinc(const RealType x) noexcept
		{
			return x == 0.0 ? 1.0 : std::sin(x * PI) / (x * PI);
		}

		[[nodiscard]] std::expected<RealType, std::string> kaiserWinCompute(RealType x) const noexcept;

		[[nodiscard]] std::expected<RealType, std::string> interpFilter(RealType x) const noexcept;

		[[nodiscard]] std::span<const RealType> getFilter(RealType delay) const;

		static InterpFilter& getInstance() noexcept;

	private:
		InterpFilter();

		RealType _alpha;
		RealType _beta = 5; // Beta sets the window shape
		RealType _bessel_beta;
		int _length;
		int _table_filters;
		std::vector<RealType> _filter_table;
	};
}
