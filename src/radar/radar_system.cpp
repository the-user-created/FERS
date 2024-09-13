// radar_system.cpp
// Implementation of classes defined in radar_system.h
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started 21 April 2006

#include "radar_system.h"

#include "core/logging.h"
#include "math_utils/multipath_surface.h"
#include "core/parameters.h"
#include "serialization/receiver_export.h"

using namespace rs;

// =====================================================================================================================
//
// RADAR CLASS
//
// =====================================================================================================================

Timing* Radar::getTiming() const
{
	if (!_timing) { throw std::runtime_error("[BUG] Radar::GetTiming called before timing set"); }
	return _timing;
}

void Radar::setMultipathDual(const RS_FLOAT reflect)
{
	_multipath_dual = true;
	_multipath_reflect = reflect;
	if (_multipath_reflect > 1)
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
	delete _timing;
}

void Receiver::addResponse(Response* response)
{
	boost::try_mutex::scoped_lock lock(_responses_mutex);
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
		boost::try_mutex::scoped_try_lock lock(_responses_mutex);
		std::sort(_responses.begin(), _responses.end(), compareTimes);
		if (parameters::exportXml()) { receiver_export::exportReceiverXml(_responses, getName() + "_results"); }
		if (parameters::exportBinary())
		{
			receiver_export::exportReceiverBinary(_responses, this, getName() + "_results");
		}
		if (parameters::exportCsv()) { receiver_export::exportReceiverCsv(_responses, getName() + "_results"); }
		lock.unlock();
	}
	catch (boost::lock_error&) { throw std::runtime_error("[BUG] Responses lock is locked during Render()"); }
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

Receiver* rs::createMultipathDual(Receiver* recv, const MultipathSurface* surf) // NOLINT(misc-no-recursion)
{
	if (recv->_dual) { return recv->_dual; }
	const Platform* dual_plat = createMultipathDual(recv->getPlatform(), surf);
	auto* dual = new Receiver(dual_plat, recv->getName() + "_dual");
	recv->_dual = dual;
	dual->_antenna = recv->_antenna;
	if (recv->_attached)
	{
		dual->_attached = createMultipathDual(dynamic_cast<Transmitter*>(const_cast<Radar*>(recv->_attached)), surf);
	}
	dual->setMultipathDual(surf->getFactor());
	dual->_noise_temperature = recv->_noise_temperature;
	dual->_window_length = recv->_window_length;
	dual->_window_prf = recv->_window_prf;
	dual->_window_skip = recv->_window_skip;
	dual->_timing = recv->_timing;
	return dual;
}

Transmitter* rs::createMultipathDual(Transmitter* trans, const MultipathSurface* surf) // NOLINT(misc-no-recursion)
{
	if (trans->_dual) { return trans->_dual; }
	const Platform* dual_plat = createMultipathDual(trans->getPlatform(), surf);
	auto* dual = new Transmitter(dual_plat, trans->getName() + "_dual", trans->_pulsed);
	trans->_dual = dual;
	dual->_antenna = trans->_antenna;
	if (trans->_attached)
	{
		dual->_attached = createMultipathDual(dynamic_cast<Receiver*>(const_cast<Radar*>(trans->_attached)), surf);
	}
	dual->setMultipathDual(surf->getFactor());
	dual->_prf = trans->_prf;
	dual->_pulsed = trans->_pulsed;
	dual->_signal = trans->_signal;
	dual->_timing = trans->_timing;
	return dual;
}
