// interpolation_filter.cpp
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "interpolation_filter.h"

#include <stdexcept>

#include "core/logging.h"
#include "core/parameters.h"

using logging::Level;

namespace interp
{
	InterpFilter& InterpFilter::getInstance()
	{
		static InterpFilter instance; // Meyers' Singleton
		return instance;
	}

	RealType InterpFilter::besselI0(const RealType x)
	{
		// Use the polynomial approximation from section 9.8 of
		// "Handbook of Mathematical Functions" by Abramowitz and Stegun
		if (x < 0.0) { throw std::logic_error("Modified Bessel approximation only valid for x > 0"); }
		if (RealType t = x / 3.75; t <= 1.0)
		{
			t *= t;
			return 1.0 + t * (
				3.5156229 + t * (3.0899424 + t * (1.2067492 + t * (0.2659732 + t * (0.0360768 + t * 0.0045813)))));
		}
		else
		{
			const RealType i0 = 0.39894228 + t * (0.01328592 + t * (
				0.00225319 + t * (-0.00157565 + t * (0.00916281 + t * (
					-0.02057706 + t * (0.02635537 + t * (-0.01647633 + t * 0.00392377)))))));
			return i0 * std::exp(x) / std::sqrt(x);
		}
	}

	RealType InterpFilter::kaiserWinCompute(const RealType x) const
	{
		return x < 0 || x > _alpha * 2
				   ? 0
				   : besselI0(_beta * std::sqrt(1 - std::pow((x - _alpha) / _alpha, 2))) / _bessel_beta;
	}

	RealType InterpFilter::interpFilter(const RealType x) const
	{
		return kaiserWinCompute(x + _alpha) * sinc(x);
	}

	InterpFilter::InterpFilter()
	{
		_length = static_cast<int>(params::renderFilterLength());
		_table_filters = 1000;
		_filter_table = std::vector<RealType>(_table_filters * _length);

		_alpha = std::floor(params::renderFilterLength() / 2.0);
		_bessel_beta = besselI0(_beta);

		const int hfilt = _table_filters / 2;

		LOG(Level::DEBUG, "Building table of {} filters", _table_filters);

		for (int i = -hfilt; i < hfilt; ++i) {
			const RealType delay = i / static_cast<RealType>(hfilt);
			for (int j = static_cast<int>(-_alpha); j < _alpha; ++j) {
				_filter_table[(i + hfilt) * _length + j + static_cast<int>(_alpha)] = interpFilter(j - delay);
			}
		}

		LOG(Level::DEBUG, "Filter table complete");
	}

	std::span<const RealType> InterpFilter::getFilter(RealType delay) const
	{
		if (delay < -1 || delay > 1)
		{
			LOG(Level::FATAL, "Invalid delay value: {}", delay);
			throw std::runtime_error("Requested delay filter value out of range");
		}

		const auto filt = static_cast<unsigned>((delay + 1) * (_table_filters / 2.0));
		return std::span{&_filter_table[filt * _length], static_cast<size_t>(_length)};
	}
}
