// radar_signal.cpp
// Classes for different types of radar waveforms
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 24 May 2006

#include "radar_signal.h"

#include <cctype>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <boost/scoped_array.hpp>

#include "hdf5_export.h"
#include "rssignal.h"

using namespace rs;

// =====================================================================================================================
//
// RADAR SIGNAL CLASS
//
// =====================================================================================================================

RadarSignal::RadarSignal(std::string name, const RS_FLOAT power, const RS_FLOAT carrierfreq, const RS_FLOAT length,
                         Signal* signal)
	: _name(std::move(name)), _power(power), _carrierfreq(carrierfreq), _length(length), _signal(signal),
	  _polar(std::complex<RS_FLOAT>(1.0, 0.0), std::complex<RS_FLOAT>(0.0, 0.0))
{
	if (!signal) { throw std::logic_error("RadarSignal cannot be constructed with NULL signal"); }
}

//Destructor
RadarSignal::~RadarSignal() { delete _signal; }

//Get the carrier frequency TODO: Cannot make this inline?????????
RS_FLOAT RadarSignal::getCarrier() const { return _carrierfreq; }

//Get the native sample rate of the pulse
RS_FLOAT RadarSignal::getRate() const { return _signal->rate(); }

std::shared_ptr<RS_COMPLEX[]> RadarSignal::render(const std::vector<InterpPoint>& points, unsigned& size,
                                                  const RS_FLOAT fracWinDelay) const
{
	std::shared_ptr<RS_COMPLEX[]> data = _signal->render(points, size, fracWinDelay);
	const RS_FLOAT scale = std::sqrt(_power);
	for (unsigned i = 0; i < size; i++) { data[i] *= scale; }
	return data;
}
