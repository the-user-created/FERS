//rsradarwaveform.cpp
//Classes for different types of radar waveforms
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 24 May 2006

#include "rsradarwaveform.h"

#include <cctype>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <boost/scoped_array.hpp>

#include "rshdf5.h"
#include "rssignal.h"

using namespace rs;

//
//RadarWaveform implementation
//

//Default constructor
RadarSignal::RadarSignal(const std::string& name, const RS_FLOAT power, const RS_FLOAT carrierfreq, const RS_FLOAT length,
                         Signal* signal):
	_name(name),
	_power(power),
	_carrierfreq(carrierfreq),
	_length(length),
	_signal(signal),
	_polar(std::complex<RS_FLOAT>(1.0, 0.0), std::complex<RS_FLOAT>(0.0, 0.0)) //Default to horiz polarization
{
	if (!signal)
	{
		throw std::logic_error("RadarSignal cannot be constructed with NULL signal");
	}
}

//Destructor
RadarSignal::~RadarSignal()
{
	delete _signal;
}

//Return the power of the signal
RS_FLOAT RadarSignal::getPower() const
{
	return _power;
}

//Get the carrier frequency
RS_FLOAT RadarSignal::getCarrier() const
{
	return _carrierfreq;
}

//Get the name of the pulse
std::string RadarSignal::getName() const
{
	return _name;
}

//Get the native sample rate of the pulse
RS_FLOAT RadarSignal::getRate() const
{
	return _signal->rate();
}

/// Return the length of the pulse
RS_FLOAT RadarSignal::getLength() const
{
	return _length;
}

/// Render the waveform to the target buffer
boost::shared_array<RsComplex> RadarSignal::render(const std::vector<InterpPoint>& points, unsigned int& size,
                                                   const RS_FLOAT fracWinDelay) const
{
	//Render the return pulse
	boost::shared_array<RsComplex> data = _signal->render(points, size, fracWinDelay);
	//Scale the return pulse by the signal power
	const RS_FLOAT scale = std::sqrt(_power);
	for (unsigned int i = 0; i < size; i++)
	{
		data[i] *= scale;
	}
	return data;
}

/// Get the signal polarization
JonesVector RadarSignal::getPolarization()
{
	return _polar;
}

/// Set the signal polarization
void RadarSignal::setPolarization(const JonesVector& in)
{
	_polar = in;
}

//
// Functions to load signal data from files
//

/// Load the pulse from HDF5 file
RadarSignal* loadPulseFromHdf5File(const std::string& name, const std::string& filename, const RS_FLOAT power,
                                   const RS_FLOAT carrierFreq)
{
	RS_FLOAT rate;
	unsigned int size;
	RsComplex* data;
	// Load the data from the hdf5 file
	rshdf5::readPulseData(filename, &data, size, rate);
	//Create the signal object
	Signal* signal = new Signal();
	// Load the pulse into the signal object
	signal->load(data, size, rate);
	delete[] data;
	// Create the RadarSignal
	RadarSignal* any = new RadarSignal(name, power, carrierFreq, size / rate, signal);
	return any;
}

/// Load the pulse from a CSV file
RadarSignal* loadPulseFromCsvFile(const std::string& name, const std::string& filename, const RS_FLOAT power,
                                  const RS_FLOAT carrierFreq)
{
	///Open the file
	std::ifstream ifile(filename.c_str());
	if (!ifile)
	{
		throw std::runtime_error("Could not open " + filename + " to read pulse waveform");
	}
	/// Read the length and sample rate
	RS_FLOAT rlength, rate;
	ifile >> rlength; //length in samples
	ifile >> rate; //rate
	const unsigned int length = static_cast<int>(rlength);
	//Allocate memory for the file contents
	const boost::scoped_array data(new RsComplex[length]);
	//Loop through reading the samples in the file
	unsigned int done = 0;
	while (!ifile.eof() && done < length)
	{
		ifile >> data[done++];
	}
	if (done != length)
	{
		throw std::runtime_error("Could not read pulse waveform from file " + filename);
	}
	//Create the signal object with the data from the file
	Signal* signal = new Signal();
	signal->load(data.get(), length, rate);
	//Create the pulse
	RadarSignal* any = new RadarSignal(name, power, carrierFreq, rlength / rate, signal);
	return any;
}

/// Load a pulse from a file and generate an anypulse
RadarSignal* rs_pulse_factory::loadPulseFromFile(const std::string& name, const std::string& filename,
                                                   const RS_FLOAT power,
                                                   const RS_FLOAT carrierFreq)
{
	//Identify file types

	//Check for CSV extension
	if (const int ln = filename.length() - 1; tolower(filename[ln]) == 'v' && tolower(filename[ln - 1]) == 's' && tolower(filename[ln - 2]) == 'c')
	{
		return loadPulseFromCsvFile(name, filename, power, carrierFreq);
	}
	//Check for H5 extension
	else
	{
		if (tolower(filename[ln]) == '5' && tolower(filename[ln - 1]) == 'h')
		{
			return loadPulseFromHdf5File(name, filename, power, carrierFreq);
		}
		// If neither of the above, complain
		throw std::runtime_error("[ERROR] Unrecognised extension while trying to load " + filename);
	}
}

