// interpolation_filter.cpp
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "interpolation_filter.h"

#include "core/logging.h"
#include "core/parameters.h"

namespace interp_filt
{
	InterpFilter* InterpFilter::getInstance()
	{
		static InterpFilter instance;  // Meyers' Singleton
		return &instance;
	}

	RS_FLOAT InterpFilter::besselI0(const RS_FLOAT x)
	{
		// Use the polynomial approximation from section 9.8 of
		// "Handbook of Mathematical Functions" by Abramowitz and Stegun
		if (x < 0.0)
		{
			throw std::logic_error("Modified Bessel approximation only valid for x > 0");
		}
		if (RS_FLOAT t = x / 3.75; t <= 1.0)
		{
			t *= t;
			return 1.0 + t * (
				3.5156229 + t * (3.0899424 + t * (1.2067492 + t * (0.2659732 + t * (0.0360768 + t * 0.0045813)))));
		}
		else
		{
			const RS_FLOAT i0 = 0.39894228 + t * (0.01328592 + t * (
				0.00225319 + t * (-0.00157565 + t * (0.00916281 + t * (
					-0.02057706 + t * (0.02635537 + t * (-0.01647633 + t * 0.00392377)))))));
			return i0 * std::exp(x) / std::sqrt(x);
		}
	}

	InterpFilter::InterpFilter()
	{
		_length = static_cast<int>(parameters::renderFilterLength());
		//Size of the table to use for interpolation
		_table_filters = 1000;
		//Allocate memory for the table
		_filter_table = std::vector<RS_FLOAT>(_table_filters * _length);
		//Alpha is half the filter length
		_alpha = std::floor(parameters::renderFilterLength() / 2.0);
		//Beta sets the window shape
		_beta = 5;
		_bessel_beta = besselI0(_beta);
		const int hfilt = _table_filters / 2;
		LOG(logging::Level::VV, "Building table of {} filters", _table_filters);
		//Fill the table of filters
		//C Tong: delay appears to be the fraction of time ellapsed between samples
		for (int i = -hfilt; i < hfilt; i++)
		{
			const RS_FLOAT delay = i / static_cast<RS_FLOAT>(hfilt);
			for (int j = static_cast<int>(-_alpha); j < _alpha; j++)
			{
				_filter_table[static_cast<int>((i + hfilt) * _length + j + _alpha)] = interpFilter(j - delay);
			}
		}
		LOG(logging::Level::VV, "Filter table complete");
	}

	const RS_FLOAT* InterpFilter::getFilter(const RS_FLOAT delay) const
	{
		const auto filt = static_cast<unsigned>((delay + 1) * (_table_filters / 2.0));

		if (delay <= -1 || delay >= 1)
		{
			LOG(logging::Level::VV, "GetFilter {} {}", delay, filt);
			throw std::runtime_error("[BUG] Requested delay filter value out of range");
		}

		return &_filter_table[filt * _length];
	}
}
