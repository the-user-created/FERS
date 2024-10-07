// radar_obj.cpp
// Implementation of classes defined in radar_obj.h
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started 21 April 2006

#include "radar_obj.h"

#include <stack>

#include "receiver.h"
#include "transmitter.h"
#include "antenna/antenna_factory.h"
#include "core/logging.h"
#include "math/multipath_surface.h"

using logging::Level;
using math::MultipathSurface;

namespace radar
{
	// =================================================================================================================
	//
	// RADAR CLASS
	//
	// =================================================================================================================
	RealType Radar::getGain(const math::SVec3& angle, const math::SVec3& refangle,
	                        const RealType wavelength) const { return _antenna->getGain(angle, refangle, wavelength); }

	RealType Radar::getNoiseTemperature(const math::SVec3& angle) const noexcept { return _antenna->getNoiseTemperature(angle); }

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

	void Radar::setMultipathDual(const RealType reflect) noexcept
	{
		_multipath_dual = true;
		_multipath_factor = reflect;
		if (_multipath_factor > 1)
		{
			LOG(Level::WARNING,
			    "Multipath reflection factor greater than 1 (={}) for radar {}, results are likely to be incorrect",
			    reflect, getName());
		}
	}

	// =================================================================================================================
	//
	// MULTIPATH DUAL FUNCTION
	//
	// =================================================================================================================

	template <typename T>
	T* createMultipathDualBase(T* obj, const MultipathSurface* surf) // NOLINT(misc-no-recursion)
	{
		if (obj->getDual())
		{
			return obj->getDual(); // Return existing dual if already created
		}

		// Create or retrieve the dual platform
		Platform* dual_platform = createMultipathDual(obj->getPlatform(), surf);

		const std::string dual_name_suffix = "_dual";

		// Create the dual object
		std::unique_ptr<T> dual;
		if constexpr (std::is_same_v<T, Transmitter>)
		{
			dual = std::make_unique<Transmitter>(dual_platform, obj->getName() + dual_name_suffix, obj->getPulsed());
		}
		else { dual = std::make_unique<T>(dual_platform, obj->getName() + dual_name_suffix); }

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
			Radar* attached = radar_stack.top();
			radar_stack.pop();

			if (auto* trans = dynamic_cast<Transmitter*>(attached))
			{
				// Create dual for attached Transmitter and link it to the dual of the current object
				dual->setAttached(createMultipathDualBase(trans, surf));
			}
			else if (auto* recv = dynamic_cast<Receiver*>(attached))
			{
				// Create dual for attached Receiver and link it to the dual of the current object
				dual->setAttached(createMultipathDualBase(recv, surf));
			}
		}

		return dual.release(); // Return the raw pointer and release ownership
	}

	// Explicit template instantiation
	template Transmitter* createMultipathDualBase(Transmitter*, const MultipathSurface*);

	template Receiver* createMultipathDualBase(Receiver*, const MultipathSurface*);
}
