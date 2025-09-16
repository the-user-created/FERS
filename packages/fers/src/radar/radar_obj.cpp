/**
 * radar_obj.cpp
 * @file
 * @brief Implementation of classes defined in radar_obj.h
 *
 * @authors David Young, Marc Brooker
 * @date 2006-04-21
 */

#include "radar_obj.h"

#include <stack>

#include "receiver.h"
#include "transmitter.h"
#include "antenna/antenna_factory.h"
#include "core/logging.h"

using logging::Level;

namespace radar
{
	RealType Radar::getGain(const math::SVec3& angle, const math::SVec3& refangle,
	                        const RealType wavelength) const { return _antenna->getGain(angle, refangle, wavelength); }

	RealType Radar::getNoiseTemperature(const math::SVec3& angle) const noexcept
	{
		return _antenna->getNoiseTemperature(angle);
	}

	void Radar::setTiming(const std::shared_ptr<timing::Timing>& tim)
	{
		if (!tim)
		{
			LOG(Level::FATAL, "Radar timing source must not be null");
			throw std::runtime_error("Radar timing source must not be null");
		}
		_timing = tim;
	}

	void Radar::setAntenna(const antenna::Antenna* ant)
	{
		if (!ant)
		{
			LOG(Level::FATAL, "Transmitter's antenna set to null");
			throw std::logic_error("Transmitter's antenna set to null");
		}
		_antenna = ant;
	}

	void Radar::setAttached(const Radar* obj)
	{
		if (_attached)
		{
			LOG(Level::FATAL, "Attempted to attach second object to transmitter");
			throw std::runtime_error("Attempted to attach second object to transmitter");
		}
		_attached = obj;
	}

	std::shared_ptr<timing::Timing> Radar::getTiming() const
	{
		if (!_timing)
		{
			LOG(Level::FATAL, "Radar::GetTiming called before timing set");
			throw std::runtime_error("Radar::GetTiming called before timing set");
		}
		return _timing;
	}
}
