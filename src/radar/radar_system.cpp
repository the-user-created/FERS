// radar_system.cpp
// Implementation of classes defined in radar_system.h
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started 21 April 2006

#include "radar_system.h"

#include <algorithm>
#include <stack>

#include "core/logging.h"
#include "core/parameters.h"
#include "math_utils/multipath_surface.h"
#include "serialization/receiver_export.h"

using namespace rs;

// =====================================================================================================================
//
// RADAR CLASS
//
// =====================================================================================================================

std::shared_ptr<Timing> Radar::getTiming() const
{
	if (!_timing) { throw std::runtime_error("[BUG] Radar::GetTiming called before timing set"); }
	return _timing;
}

void Radar::setMultipathDual(const RS_FLOAT reflect)
{
	_multipath_dual = true;
	_multipath_factor = reflect;
	if (_multipath_factor > 1)
	{
		logging::printf(logging::RS_CRITICAL,
		                "[CRITICAL] Multipath reflection factor greater than 1 (=%g) for radar %s, results are likely to be incorrect\n",
		                reflect, getName().c_str());
	}
}

// =====================================================================================================================
//
// TRANSMITTER CLASS
//
// =====================================================================================================================

int Transmitter::getPulseCount() const
{
	const RS_FLOAT time = parameters::endTime() - parameters::startTime();
	if (_pulsed)
	{
		const RS_FLOAT pulses = time * _prf;
		return static_cast<int>(std::ceil(pulses));
	}
	return 1; // CW systems only have one 'pulse'
}

void Transmitter::getPulse(TransmitterPulse* pulse, const int number) const
{
	pulse->wave = _signal;
	pulse->time = _pulsed ? static_cast<RS_FLOAT>(number) / _prf : 0;
	if (!_timing)
	{
		throw std::logic_error("[BUG] Transmitter " + getName() + " must be associated with timing source");
	}
}

void Transmitter::setPrf(const RS_FLOAT mprf)
{
	const RS_FLOAT rate = parameters::rate() * parameters::oversampleRatio();
	_prf = 1 / (std::floor(rate / mprf) / rate);
}

// =====================================================================================================================
//
// RECEIVER CLASS
//
// =====================================================================================================================

Receiver::~Receiver()
{
	clearResponses();
}

void Receiver::addResponse(Response* response)
{
	std::unique_lock lock(_responses_mutex);
	_responses.push_back(response);
}

void Receiver::clearResponses()
{
	for (const auto& response : _responses) { delete response; }
	_responses.clear();
}

void Receiver::render()
{
	try
	{
		std::unique_lock lock(_responses_mutex, std::try_to_lock);
		if (!lock.owns_lock()) { throw std::runtime_error("[BUG] Responses lock is locked during Render()"); }
		std::sort(_responses.begin(), _responses.end(), compareTimes);
		if (parameters::exportXml()) { receiver_export::exportReceiverXml(_responses, getName() + "_results"); }
		if (parameters::exportBinary())
		{
			receiver_export::exportReceiverBinary(_responses, this, getName() + "_results");
		}
		if (parameters::exportCsv()) { receiver_export::exportReceiverCsv(_responses, getName() + "_results"); }
		lock.unlock();
	}
	catch (const std::system_error&) { throw std::runtime_error("[BUG] Responses lock is locked during Render()"); }
}

void Receiver::setWindowProperties(const RS_FLOAT length, const RS_FLOAT prf, const RS_FLOAT skip)
{
	const RS_FLOAT rate = parameters::rate() * parameters::oversampleRatio();
	_window_length = length;
	_window_prf = prf;
	_window_skip = skip;
	_window_prf = 1 / (std::floor(rate / _window_prf) / rate);
	_window_skip = std::floor(rate * _window_skip) / rate;
}

int Receiver::getWindowCount() const
{
	const RS_FLOAT time = parameters::endTime() - parameters::startTime();
	const RS_FLOAT pulses = time * _window_prf;
	return static_cast<int>(std::ceil(pulses));
}

RS_FLOAT Receiver::getWindowStart(const int window) const
{
	const RS_FLOAT stime = static_cast<RS_FLOAT>(window) / _window_prf + _window_skip;
	if (!_timing) { throw std::logic_error("[BUG] Receiver must be associated with timing source"); }
	return stime;
}

// =====================================================================================================================
//
// MULTIPATH DUAL FUNCTIONS
//
// =====================================================================================================================

template <typename T>
T* createMultipathDualBase(T* obj, const MultipathSurface* surf, const std::string& dualNameSuffix) // NOLINT(misc-no-recursion)
{
	if (obj->getDual()) {
		return obj->getDual();
	}

	logging::printf(logging::RS_VERBOSE, "[%s.createMultipathDual] Creating dual for %s\n",
					typeid(T).name(), obj->getName().c_str());

	// Create or retrieve the dual platform
	Platform* dual_platform = createMultipathDual(obj->getPlatform(), surf);

	// Handle the specific case for Transmitter that needs an extra 'pulsed' parameter
	T* dual = nullptr;
	if constexpr (std::is_same_v<T, Transmitter>) {
		dual = new Transmitter(dual_platform, obj->getName() + dualNameSuffix, obj->getPulsed());
	} else {
		dual = new T(dual_platform, obj->getName() + dualNameSuffix);
	}

	// Link the dual to the original
	obj->setDual(dual);

	// Set shared properties
	dual->setAntenna(obj->getAntenna());
	dual->setTiming(obj->getTiming());

	if constexpr (std::is_same_v<T, Receiver>) {
		dual->setNoiseTemperature(obj->getNoiseTemperature());
		dual->setWindowProperties(obj->getWindowLength(), obj->getWindowPrf(), obj->getWindowSkip());
	} else if constexpr (std::is_same_v<T, Transmitter>) {
		dual->setPrf(obj->getPrf());
		dual->setSignal(obj->getSignal());
		dual->setPulsed(obj->getPulsed());
	}

	// Set multipath factor
	dual->setMultipathDual(surf->getFactor());

	// Use a stack to iteratively process attached objects
	std::stack<Radar*> stack;

	// If the object has an attached component, push it onto the stack for further processing
	if (obj->getAttached()) {
		stack.push(const_cast<Radar*>(obj->getAttached()));
	}

	while (!stack.empty()) {
		// print stack size
		logging::printf(logging::RS_VERBOSE, "[%s.createMultipathDual] Stack size: %d\n",
						typeid(T).name(), stack.size());
		Radar* attached = stack.top();
		stack.pop();

		if (auto* trans = dynamic_cast<Transmitter*>(attached)) {
			// Create dual for attached Transmitter and link it to the dual of the current object
			dual->setAttached(createMultipathDualBase(trans, surf, dualNameSuffix));
		} else if (auto* recv = dynamic_cast<Receiver*>(attached)) {
			// Create dual for attached Receiver and link it to the dual of the current object
			dual->setAttached(createMultipathDualBase(recv, surf, dualNameSuffix));
		}
	}

	return dual;
}


Receiver* rs::createMultipathDual(Receiver* recv, const MultipathSurface* surf)
{
	return createMultipathDualBase(recv, surf, "_dual");
}


Transmitter* rs::createMultipathDual(Transmitter* trans, const MultipathSurface* surf)
{
	return createMultipathDualBase(trans, surf, "_dual");
}
