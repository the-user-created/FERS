//rsradar.cpp
//Implementation of classes defined in rsradar.h
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started 21 April 2006

#include "rsradar.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "rsantenna.h"
#include "rsdebug.h"
#include "rsmultipath.h"
#include "rsparameters.h"
#include "rsplatform.h"
#include "rspulserender.h"
#include "rsresponse.h"
#include "rstiming.h"

using namespace rs; //Import the rs namespace for clarity

//
// Radar Implementation
//

/// Default Constructor
Radar::Radar(const Platform* platform, const std::string& name):
	Object(platform, name),
	_timing(nullptr),
	_antenna(nullptr),
	_attached(nullptr),
	_multipath_dual(false),
	_multipath_reflect(0)
{
}

/// Default Destructor
Radar::~Radar()
{
}

/// Attach a receiver to the transmitter for a monostatic configuration
void Radar::makeMonostatic(const Radar* recv)
{
	if (_attached)
	{
		throw std::runtime_error("[BUG] Attempted to attach second receiver to transmitter");
	}
	_attached = recv;
}

/// Get the attached receiver
//Attached is likely to be 0 (NULL) - which means the transmitter does not share it's antenna
const Radar* Radar::getAttached() const
{
	return _attached;
}

/// Return whether the radar is monostatic
bool Radar::isMonostatic() const
{
	return _attached;
}

/// Set the transmitter's antenna
void Radar::setAntenna(const Antenna* ant)
{
	if (!ant)
	{
		throw std::logic_error("[BUG] Transmitter's antenna set to null");
	}
	_antenna = ant;
}

/// Return the antenna gain in the specified direction
RS_FLOAT Radar::getGain(const SVec3& angle, const SVec3& refangle, const RS_FLOAT wavelength) const
{
	return _antenna->getGain(angle, refangle, wavelength);
}

/// Get the noise temperature (including antenna noise temperature)
RS_FLOAT Radar::getNoiseTemperature(const SVec3& angle) const
{
	return _antenna->getNoiseTemperature(angle);
}

/// Attach a timing object to the radar
void Radar::setTiming(Timing* tim)
{
	if (!tim)
	{
		throw std::runtime_error("[BUG] Radar timing source must not be set to NULL");
	}
	_timing = tim;
}

/// Get a pointer to the radar's timing object
Timing* Radar::getTiming() const
{
	if (!_timing)
	{
		throw std::runtime_error("[BUG] Radar::GetTiming called before timing set");
	}
	return _timing;
}

/// Is this object a virtual multipath dual?
bool Radar::isMultipathDual() const
{
	return _multipath_dual;
}

/// Set this object as a virtual multipath dual
void Radar::setMultipathDual(const RS_FLOAT reflect)
{
	_multipath_dual = true;
	_multipath_reflect = reflect;
	// Sanity check the reflectance factor
	if (_multipath_reflect > 1)
	{
		rs_debug::printf(rs_debug::RS_CRITICAL,
		                 "[CRITICAL] Multipath reflection factor greater than 1 (=%g) for radar %s, results are likely to be incorrect\n",
		                 reflect, getName().c_str());
	}
}

/// Get the reflecting factor
RS_FLOAT Radar::multipathDualFactor() const
{
	return _multipath_reflect;
}

//
// Transmitter Implementation
//

//Default constructor for Transmitter
Transmitter::Transmitter(const Platform* platform, const std::string& name, const bool pulsed):
	Radar(platform, name),
	_signal(nullptr),
	_pulsed(pulsed),
	_dual(nullptr)
{
}

//Default destructor for Transmitter
Transmitter::~Transmitter()
{
	delete getTiming();
}

/// Set the transmitter's pulse waveform
void Transmitter::setWave(RadarSignal* pulse)
{
	_signal = pulse;
}

// Return the number of pulses this transmitter produces over the simulation lifetime
int Transmitter::getPulseCount() const
{
	const RS_FLOAT time = RsParameters::endTime() - RsParameters::startTime();
	if (_pulsed)
	{
		const RS_FLOAT pulses = time * _prf;
		return static_cast<int>(std::ceil(pulses));
	}
	else
	{
		return 1; //CW systems only have one 'pulse'
	}
}

// Fill the structure with the number'th pulse in the transmitter's pulse list
void Transmitter::getPulse(TransmitterPulse* pulse, const int number) const
{
	//Pulse waveform is same as transmitter waveform
	pulse->wave = _signal;
	//Calculate start time of pulse
	if (_pulsed)
	{
		pulse->time = static_cast<RS_FLOAT>(number) / _prf; //Pulse mode start depends on PRF
	}
	else
	{
		pulse->time = 0; //CW transmitters start at zero for now
	}
	//If there is timing jitter, add it
	if (!_timing)
	{
		throw std::logic_error("[BUG] Transmitter " + getName() + " must be associated with timing source");
	}
	//pulse->time = pulse->time;//+timing->GetPulseTimeError();
}

/// Set the Pulse Repetition Frequency of the transmitter
void Transmitter::setPrf(const RS_FLOAT mprf)
{
	const RS_FLOAT rate = RsParameters::rate() * RsParameters::oversampleRatio();
	// The PRF must be rounded to an even number of samples
	_prf = 1 / (std::floor(rate / mprf) / rate);
}

//
// Receiver Implementation
//

//Default constructor for Receiver
Receiver::Receiver(const Platform* platform, const std::string& name):
	Radar(platform, name),
	_noise_temperature(0),
	_dual(nullptr),
	_flags(0)
{
}

//Default destructor for Receiver
Receiver::~Receiver()
{
	clearResponses();
	delete _timing; //The timing is unique to the receiver
}

//Add a response to the list of responses for this receiver
void Receiver::addResponse(Response* response)
{
	boost::try_mutex::scoped_lock lock(_responses_mutex);
	_responses.push_back(response);
}

//Clear the list of system responses
void Receiver::clearResponses()
{
	for (std::vector<Response*>::iterator i = _responses.begin(); i != _responses.end(); ++i)
	{
		delete *i;
	}
	_responses.clear();
}

/// Comparison function for response*
inline bool compareTimes(const Response* a, const Response* b)
{
	return (a->startTime()) < (b->startTime());
}

/// Render the antenna's responses
void Receiver::render()
{
	try
	{
		// This mutex should never be locked, enforce that condition
		boost::try_mutex::scoped_try_lock lock(_responses_mutex);
		//Sort the returns into time order
		std::sort(_responses.begin(), _responses.end(), compareTimes);
		//Export the pulse descriptions to XML
		if (RsParameters::exportXml())
		{
			exportReceiverXml(_responses, getName() + "_results");
		}
		//Export a binary containing the pulses
		if (RsParameters::exportBinary())
		{
			exportReceiverBinary(_responses, this, getName(), getName() + "_results");
		}
		//Export to CSV format
		if (RsParameters::exportCsv())
		{
			exportReceiverCsv(_responses, getName() + "_results");
		}
		//Unlock the mutex
		lock.unlock();
	}
	catch (boost::lock_error& e)
	{
		throw std::runtime_error("[BUG] Responses lock is locked during Render()");
	}
}

/// Get the noise temperature (including antenna noise temperature)
RS_FLOAT Receiver::getNoiseTemperature(const SVec3& angle) const
{
	return _noise_temperature + Radar::getNoiseTemperature(angle);
}

/// Get the receiver noise temperature
RS_FLOAT Receiver::getNoiseTemperature() const
{
	return _noise_temperature;
}

/// Set the noise temperature of the receiver
void Receiver::setNoiseTemperature(const RS_FLOAT temp)
{
	if (temp < -std::numeric_limits<RS_FLOAT>::epsilon())
	{
		throw std::runtime_error("Noise temperature set to negative value.");
	}
	_noise_temperature = temp;
}

/// Set the length of the receive window
void Receiver::setWindowProperties(const RS_FLOAT length, const RS_FLOAT prf, const RS_FLOAT skip)
{
	const RS_FLOAT rate = RsParameters::rate() * RsParameters::oversampleRatio();
	_window_length = length;
	_window_prf = prf;
	_window_skip = skip;
	// The PRF and skip must be rounded to an even number of samples
	_window_prf = 1 / (std::floor(rate / _window_prf) / rate);
	_window_skip = std::floor(rate * _window_skip) / rate;
}

/// Return the number of responses
int Receiver::countResponses() const
{
	return _responses.size();
}

/// Get the number of receive windows in the simulation time
int Receiver::getWindowCount() const
{
	const RS_FLOAT time = RsParameters::endTime() - RsParameters::startTime();
	const RS_FLOAT pulses = time * _window_prf;
	return static_cast<int>(std::ceil(pulses));
}

/// Get the start time of the next window
RS_FLOAT Receiver::getWindowStart(const int window) const
{
	//Calculate start time of pulse
	const RS_FLOAT stime = static_cast<RS_FLOAT>(window) / _window_prf + _window_skip;
	//If there is timing jitter, add it
	if (!_timing)
	{
		throw std::logic_error("[BUG] Receiver must be associated with timing source");
	}
	//stime = stime;//+timing->GetPulseTimeError();
	return stime;
}

/// Get the length of the receive window
RS_FLOAT Receiver::getWindowLength() const
{
	return _window_length;
}

/// Get the time skipped before the start of the receive window
RS_FLOAT Receiver::getWindowSkip() const
{
	return _window_skip;
}

/// Get the length of the receive window
RS_FLOAT Receiver::getPrf() const
{
	return _window_prf;
}

/// Set a flag
void Receiver::setFlag(const RecvFlag flag)
{
	_flags |= flag;
}

/// Check if a flag is set
bool Receiver::checkFlag(const RecvFlag flag) const
{
	return _flags & flag;
}


//
// Multipath dual functions
//

// Create a multipath dual of the given receiver
Receiver* rs::createMultipathDual(Receiver* recv, const MultipathSurface* surf)
{
	//If we already have a dual, simply return the pointer to it
	if (recv->_dual)
	{
		return recv->_dual;
	}
	//Get the dual platform
	const Platform* dual_plat = createMultipathDual(recv->getPlatform(), surf);
	//Create a new receiver object
	Receiver* dual = new Receiver(dual_plat, recv->getName() + "_dual");
	//Assign the new receiver object to the current object
	recv->_dual = dual;
	//Copy data from the Radar object
	dual->_antenna = recv->_antenna;
	if (recv->_attached)
	{
		dual->_attached = createMultipathDual(dynamic_cast<Transmitter*>(const_cast<Radar*>(recv->_attached)), surf);
	}
	dual->setMultipathDual(surf->getFactor());
	//Copy data from the receiver object
	dual->_noise_temperature = recv->_noise_temperature;
	dual->_window_length = recv->_window_length;
	dual->_window_prf = recv->_window_prf;
	dual->_window_skip = recv->_window_skip;
	dual->_timing = recv->_timing;
	//Done, return the created object
	return dual;
}

// Create a multipath dual of the given transmitter
Transmitter* rs::createMultipathDual(Transmitter* trans, const MultipathSurface* surf)
{
	//If we already have a dual, simply return a pointer to it
	if (trans->_dual)
	{
		return trans->_dual;
	}
	//Get the dual platform
	const Platform* dual_plat = createMultipathDual(trans->getPlatform(), surf);
	//Create a new transmitter object
	Transmitter* dual = new Transmitter(dual_plat, trans->getName() + "_dual", trans->_pulsed);
	//Assign the the transmitter object to the current object
	trans->_dual = dual;
	//Copy data from the Radar object
	dual->_antenna = trans->_antenna;
	if (trans->_attached)
	{
		dual->_attached = createMultipathDual(dynamic_cast<Receiver*>(const_cast<Radar*>(trans->_attached)), surf);
	}
	dual->setMultipathDual(surf->getFactor());
	//Copy data from the transmitter object
	dual->_prf = trans->_prf;
	dual->_pulsed = trans->_pulsed;
	dual->_signal = trans->_signal;
	dual->_timing = trans->_timing;
	//Done, return the created object
	return dual;
}
