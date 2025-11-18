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

#include <libfers/transmitter.h>

#include <libfers/parameters.h>

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
}
