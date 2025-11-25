// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
 * @file transmitter.cpp
 * @brief Contains the implementation of the Transmitter class.
 */

#include "transmitter.h"

#include "core/parameters.h"

namespace radar
{
	void Transmitter::setPulse(TransmitterPulse* pulse, const int number) const
	{
		pulse->wave = _signal;
		pulse->time = _mode == OperationMode::PULSED_MODE ? static_cast<RealType>(number) / _prf : 0;
		if (!_timing)
		{
			LOG(logging::Level::FATAL, "Transmitter {} must be associated with timing source", getName());
			throw std::logic_error("Transmitter " + getName() + " must be associated with timing source");
		}
	}

	void Transmitter::setPrf(const RealType mprf) noexcept
	{
		const RealType rate = params::rate() * params::oversampleRatio();
		_prf = 1 / (std::floor(rate / mprf) / rate);
	}

	void Transmitter::addSchedulePeriod(RealType start, RealType end)
	{
		_schedule.push_back({start, end});
		// Ensure schedule is sorted for efficient lookup
		std::sort(_schedule.begin(), _schedule.end(),
				  [](const SchedulePeriod& a, const SchedulePeriod& b) { return a.start < b.start; });
	}

	std::optional<RealType> Transmitter::getNextPulseTime(RealType time) const
	{
		// If no schedule is defined, assume always on.
		if (_schedule.empty())
		{
			return time;
		}

		for (const auto& period : _schedule)
		{
			// If time is within this period, it's valid.
			if (time >= period.start && time <= period.end)
			{
				return time;
			}
			// If time is before this period, skip to the start of this period.
			if (time < period.start)
			{
				return period.start;
			}
			// If time is after this period, continue to next period.
		}

		// Time is after the last scheduled period.
		return std::nullopt;
	}
}
