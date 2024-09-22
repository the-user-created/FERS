// radar_system.cpp
// Implementation of classes defined in radar_system.h
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started 21 April 2006

#include "radar_system.h"

#include <algorithm>                        // for __sort_fn, sort
#include <cmath>                            // for floor, ceil
#include <limits>                           // for numeric_limits
#include <span>                             // for span
#include <stack>                            // for stack
#include <stdexcept>                        // for runtime_error, logic_error
#include <system_error>                     // for system_error
#include <typeinfo>                         // for type_info

#include "antenna/antenna_factory.h"        // for Antenna
#include "core/logging.h"                   // for log, LOG, Level
#include "core/parameters.h"                // for endTime, oversampleRatio
#include "math_utils/multipath_surface.h"   // for MultipathSurface
#include "radar/platform.h"                 // for createMultipathDual, Plat...
#include "serialization/receiver_export.h"  // for exportReceiverBinary, exp...

using logging::Level;
using math::MultipathSurface;

namespace math
{
	class SVec3;
}

namespace radar
{
	// =================================================================================================================
	//
	// RADAR CLASS
	//
	// =================================================================================================================
	RealType Radar::getGain(const math::SVec3& angle, const math::SVec3& refangle,
	                        const RealType wavelength) const { return _antenna->getGain(angle, refangle, wavelength); }

	RealType Radar::getNoiseTemperature(const math::SVec3& angle) const { return _antenna->getNoiseTemperature(angle); }

	void Radar::setTiming(const std::shared_ptr<timing::Timing>& tim)
	{
		if (!tim) { throw std::runtime_error("[BUG] Radar timing source must not be null"); }
		_timing = tim;
	}

	void Radar::setAntenna(const antenna::Antenna* ant)
	{
		if (!ant) { throw std::logic_error("[BUG] Transmitter's antenna set to null"); }
		_antenna = ant;
	}

	void Radar::setAttached(const Radar* obj)
	{
		if (_attached) { throw std::runtime_error("[BUG] Attempted to attach second object to transmitter"); }
		_attached = obj;
	}

	std::shared_ptr<timing::Timing> Radar::getTiming() const
	{
		if (!_timing) { throw std::runtime_error("[BUG] Radar::GetTiming called before timing set"); }
		return _timing;
	}

	void Radar::setMultipathDual(const RealType reflect)
	{
		_multipath_dual = true;
		_multipath_factor = reflect;
		if (_multipath_factor > 1)
		{
			LOG(Level::ERROR,
			    "Multipath reflection factor greater than 1 (={}) for radar {}, results are likely to be incorrect",
			    reflect, getName());
		}
	}

	// =================================================================================================================
	//
	// TRANSMITTER CLASS
	//
	// =================================================================================================================

	int Transmitter::getPulseCount() const
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
			throw std::logic_error("[BUG] Transmitter " + getName() + " must be associated with timing source");
		}
	}

	void Transmitter::setPrf(const RealType mprf)
	{
		const RealType rate = params::rate() * params::oversampleRatio();
		_prf = 1 / (std::floor(rate / mprf) / rate);
	}

	// =================================================================================================================
	//
	// RECEIVER CLASS
	//
	// =================================================================================================================

	void Receiver::addResponse(std::unique_ptr<serial::Response> response)
	{
		std::lock_guard lock(_responses_mutex);
		_responses.push_back(std::move(response));
	}

	RealType Receiver::getNoiseTemperature(const math::SVec3& angle) const
	{
		return _noise_temperature + Radar::getNoiseTemperature(angle);
	}

	int Receiver::getResponseCount() const
	{
		std::lock_guard lock(_responses_mutex);
		return static_cast<int>(_responses.size());
	}

	void Receiver::setNoiseTemperature(const RealType temp)
	{
		if (temp < -std::numeric_limits<RealType>::epsilon())
		{
			throw std::runtime_error("[BUG] Noise temperature must be positive");
		}
		_noise_temperature = temp;
	}

	void Receiver::render()
	{
		try
		{
			std::unique_lock lock(_responses_mutex, std::try_to_lock);
			if (!lock.owns_lock()) { throw std::runtime_error("[BUG] Responses lock is locked during Render()"); }

			std::ranges::sort(_responses, compareTimes);

			// Export based on user preferences
			if (params::exportXml()) { exportReceiverXml(_responses, getName() + "_results"); }
			if (params::exportBinary()) { exportReceiverBinary(_responses, this, getName() + "_results"); }
			if (params::exportCsv()) { exportReceiverCsv(_responses, getName() + "_results"); }

			lock.unlock();
		}
		catch (const std::system_error&) { throw std::runtime_error("[BUG] Responses lock is locked during Render()"); }
	}

	void Receiver::setWindowProperties(const RealType length, const RealType prf, const RealType skip)
	{
		const auto rate = params::rate() * params::oversampleRatio();
		_window_length = length;
		_window_prf = 1 / (std::floor(rate / prf) / rate); // Update prf to precise value
		_window_skip = std::floor(rate * skip) / rate; // Update skip with better precision
	}

	int Receiver::getWindowCount() const
	{
		const RealType time = params::endTime() - params::startTime();
		const RealType pulses = time * _window_prf;
		return static_cast<int>(std::ceil(pulses));
	}

	RealType Receiver::getWindowStart(const int window) const
	{
		const RealType stime = static_cast<RealType>(window) / _window_prf + _window_skip;
		if (!_timing) { throw std::logic_error("[BUG] Receiver must be associated with timing source"); }
		return stime;
	}

	// =================================================================================================================
	//
	// MULTIPATH DUAL FUNCTIONS
	//
	// =================================================================================================================

	template <typename T>
	T* createMultipathDualBase(T* obj, const MultipathSurface* surf, const std::string& dualNameSuffix)
	// NOLINT(misc-no-recursion)
	{
		if (obj->getDual())
		{
			return obj->getDual(); // Return existing dual if already created
		}

		LOG(Level::DEBUG, "[{}.createMultipathDual] Creating dual for {}",
		    typeid(T).name(), obj->getName());

		// Create or retrieve the dual platform
		Platform* dual_platform = createMultipathDual(obj->getPlatform(), surf);

		// Create the dual object
		std::unique_ptr<T> dual;
		if constexpr (std::is_same_v<T, Transmitter>)
		{
			dual = std::make_unique<Transmitter>(dual_platform, obj->getName() + dualNameSuffix, obj->getPulsed());
		}
		else { dual = std::make_unique<T>(dual_platform, obj->getName() + dualNameSuffix); }

		// Link the dual to the original
		obj->setDual(dual.get());

		// Set shared properties
		dual->setAntenna(obj->getAntenna());
		dual->setTiming(obj->getTiming());

		if constexpr (std::is_same_v<T, Receiver>)
		{
			dual->setNoiseTemperature(obj->getNoiseTemperature());
			dual->setWindowProperties(obj->getWindowLength(), obj->getWindowPrf(), obj->getWindowSkip());
		}
		else if constexpr (std::is_same_v<T, Transmitter>)
		{
			dual->setPrf(obj->getPrf());
			dual->setSignal(obj->getSignal());
			dual->setPulsed(obj->getPulsed());
		}

		// Set multipath factor
		dual->setMultipathDual(surf->getFactor());

		// Use a stack to iteratively process attached objects
		std::stack<Radar*> radar_stack;

		if (auto* attached = obj->getAttached(); attached != nullptr)
		{
			radar_stack.push(const_cast<Radar*>(attached)); // Push the attached object for further processing
		}

		while (!radar_stack.empty())
		{
			LOG(Level::DEBUG, "[{}.createMultipathDual] Stack size: {}",
			    typeid(T).name(), radar_stack.size());

			Radar* attached = radar_stack.top();
			radar_stack.pop();

			if (auto* trans = dynamic_cast<Transmitter*>(attached))
			{
				// Create dual for attached Transmitter and link it to the dual of the current object
				dual->setAttached(createMultipathDualBase(trans, surf, dualNameSuffix));
			}
			else if (auto* recv = dynamic_cast<Receiver*>(attached))
			{
				// Create dual for attached Receiver and link it to the dual of the current object
				dual->setAttached(createMultipathDualBase(recv, surf, dualNameSuffix));
			}
		}

		return dual.release(); // Return the raw pointer and release ownership
	}

	Receiver* createMultipathDual(Receiver* recv, const MultipathSurface* surf)
	{
		return createMultipathDualBase(recv, surf, "_dual");
	}

	Transmitter* createMultipathDual(Transmitter* trans, const MultipathSurface* surf)
	{
		return createMultipathDualBase(trans, surf, "_dual");
	}
}
