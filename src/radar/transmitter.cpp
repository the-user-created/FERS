/**
 * @file transmitter.cpp
 * @brief Contains the implementation of the Transmitter class.
 *
 * @authors David Young, Marc Brooker
 * @date 2024-10-07
 */

#include "transmitter.h"

#include "core/parameters.h"

namespace radar
{
	int Transmitter::getPulseCount() const noexcept
	{
		if (!_pulsed)
		{
			return 1;
		}
		const RealType time = params::endTime() - params::startTime();
		const RealType pulses = time * _prf;
		return static_cast<int>(std::ceil(pulses));
	}

	void Transmitter::setPulse(TransmitterPulse* pulse, const int number) const
	{
		pulse->wave = _signal;
		if (_pulsed)
		{
			pulse->time = static_cast<RealType>(number) / _prf;
		}
		else
		{
			pulse->time = params::startTime();
			// pulse->wave->length = params::endTime() - params::startTime(); // Cannot be implemented without new methods.
		}

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
