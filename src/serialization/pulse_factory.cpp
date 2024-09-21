// pulse_factory.cpp
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "pulse_factory.h"

#include <complex>
#include <fstream>
#include <vector>

#include "config.h"
#include "hdf5_handler.h"
#include "signal_processing/radar_signal.h"

using signal::Signal;
using signal::RadarSignal;

namespace
{
	// =================================================================================================================
	//
	// FUNCTIONS TO LOAD SIGNAL DATA FROM FILES
	//
	// =================================================================================================================

	std::unique_ptr<RadarSignal> loadPulseFromHdf5File(const std::string& name, const std::string& filename,
	                                                   const RS_FLOAT power, const RS_FLOAT carrierFreq)
	{
		RS_FLOAT rate;
		std::vector<RS_COMPLEX> data;
		serial::readPulseData(filename, data, rate);

		auto signal = std::make_unique<Signal>();
		signal->load(data.data(), data.size(), rate);
		return std::make_unique<RadarSignal>(name, power, carrierFreq, static_cast<RS_FLOAT>(data.size()) / rate,
		                                     std::move(signal));
	}

	std::unique_ptr<RadarSignal> loadPulseFromCsvFile(const std::string& name, const std::string& filename,
	                                                  const RS_FLOAT power, const RS_FLOAT carrierFreq)
	{
		std::ifstream ifile(filename.c_str());
		if (!ifile) { throw std::runtime_error("Could not open " + filename + " to read pulse waveform"); }
		RS_FLOAT rlength, rate;
		ifile >> rlength >> rate;
		const unsigned length = static_cast<int>(rlength);
		std::vector<RS_COMPLEX> data(length);
		unsigned done = 0;
		while (!ifile.eof() && done < length) { ifile >> data[done++]; }
		if (done != length) { throw std::runtime_error("Could not read pulse waveform from file " + filename); }
		auto signal = std::make_unique<Signal>();
		signal->load(data.data(), length, rate);
		return std::make_unique<RadarSignal>(name, power, carrierFreq, rlength / rate, std::move(signal));
	}
}

namespace serial
{
	std::unique_ptr<RadarSignal> loadPulseFromFile(const std::string& name, const std::string& filename,
	                                               const RS_FLOAT power, const RS_FLOAT carrierFreq)
	{
		if (const unsigned long ln = filename.length() - 1; tolower(filename[ln]) == 'v' && tolower(filename[ln - 1]) ==
			's'
			&& tolower(filename[ln - 2]) == 'c') { return loadPulseFromCsvFile(name, filename, power, carrierFreq); }
		else
		{
			if (tolower(filename[ln]) == '5' && tolower(filename[ln - 1]) == 'h')
			{
				return loadPulseFromHdf5File(name, filename, power, carrierFreq);
			}
			throw std::runtime_error("Unrecognised extension while trying to load " + filename);
		}
	}
}
