// pulse_factory.cpp
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "pulse_factory.h"

#include <fstream>
#include <boost/scoped_array.hpp>

#include "hdf5_export.h"
#include "radar_signal.h"

// =====================================================================================================================
//
// FUNCTIONS TO LOAD SIGNAL DATA FROM FILES
//
// =====================================================================================================================

rs::RadarSignal* loadPulseFromHdf5File(const std::string& name, const std::string& filename, const RS_FLOAT power,
                                       const RS_FLOAT carrierFreq)
{
	RS_FLOAT rate;
	unsigned size;
	RS_COMPLEX* data;
	hdf5_export::readPulseData(filename, &data, size, rate);
	auto* signal = new rs::Signal();
	signal->load(data, size, rate);
	delete[] data;
	return new rs::RadarSignal(name, power, carrierFreq, size / rate, signal);
}

rs::RadarSignal* loadPulseFromCsvFile(const std::string& name, const std::string& filename, const RS_FLOAT power,
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
	auto* signal = new rs::Signal();
	signal->load(data.get(), length, rate);
	return new rs::RadarSignal(name, power, carrierFreq, rlength / rate, signal);
}

rs::RadarSignal* pulse_factory::loadPulseFromFile(const std::string& name, const std::string& filename,
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
