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

// =====================================================================================================================
//
// FUNCTIONS TO LOAD SIGNAL DATA FROM FILES TODO: Should be moved to a separate file
//
// =====================================================================================================================

RadarSignal* loadPulseFromHdf5File(const std::string& name, const std::string& filename, const RS_FLOAT power,
                                   const RS_FLOAT carrierFreq)
{
	RS_FLOAT rate;
	unsigned size;
	RS_COMPLEX* data;
	hdf5_export::readPulseData(filename, &data, size, rate);
	auto* signal = new Signal();
	signal->load(data, size, rate);
	delete[] data;
	return new RadarSignal(name, power, carrierFreq, size / rate, signal);
}

RadarSignal* loadPulseFromCsvFile(const std::string& name, const std::string& filename, const RS_FLOAT power,
                                  const RS_FLOAT carrierFreq)
{
	std::ifstream ifile(filename.c_str());
	if (!ifile) { throw std::runtime_error("Could not open " + filename + " to read pulse waveform"); }
	RS_FLOAT rlength, rate;
	ifile >> rlength >> rate;
	const unsigned length = static_cast<int>(rlength);
	const boost::scoped_array data(new RS_COMPLEX[length]);
	unsigned done = 0;
	while (!ifile.eof() && done < length) { ifile >> data[done++]; }
	if (done != length) { throw std::runtime_error("Could not read pulse waveform from file " + filename); }
	auto* signal = new Signal();
	signal->load(data.get(), length, rate);
	return new RadarSignal(name, power, carrierFreq, rlength / rate, signal);
}

RadarSignal* pulse_factory::loadPulseFromFile(const std::string& name, const std::string& filename,
                                              const RS_FLOAT power, const RS_FLOAT carrierFreq)
{
	if (const unsigned long ln = filename.length() - 1; tolower(filename[ln]) == 'v' && tolower(filename[ln - 1]) == 's'
		&& tolower(filename[ln - 2]) == 'c') { return loadPulseFromCsvFile(name, filename, power, carrierFreq); }
	else
	{
		if (tolower(filename[ln]) == '5' && tolower(filename[ln - 1]) == 'h')
		{
			return loadPulseFromHdf5File(name, filename, power, carrierFreq);
		}
		throw std::runtime_error("[ERROR] Unrecognised extension while trying to load " + filename);
	}
}

