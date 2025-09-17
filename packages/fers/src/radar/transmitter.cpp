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
	int Transmitter::getPulseCount() const noexcept
	{
		const RealType time = params::endTime() - params::startTime();
		if (_pulsed)
		{
			const RealType pulses = time * _prf;
			return static_cast<int>(std::ceil(pulses));
		}
		return 1; // CW systems only have one 'pulse'
	}

	void Transmitter::setPulse(TransmitterPulse* pulse, const int number) const
	{
		pulse->wave = _signal;
		pulse->time = _pulsed ? static_cast<RealType>(number) / _prf : 0;
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
}
