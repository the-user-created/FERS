//rsparameters.cpp
//Implementation of Singleton class to hold common simulation parameters
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//11 June 2006

#include "rsparameters.h"

#include <ctime>
#include <stdexcept>

#include "rsdebug.h"

using namespace rs;

RsParameters* RsParameters::_instance = new RsParameters();

namespace
{
	struct SimParameters
	{
		rsFloat c{}; //!< Propagation speed of the wave in the medium
		rsFloat start{}; //!< The start time of the simulation
		rsFloat end{}; //!< The end time of the simulation
		rsFloat cw_sample_rate{}; //<! The number of samples per second to take of changes in the CW state
		rsFloat rate{}; //!< The sample rate to use for rendering
		unsigned int random_seed{}; //!< The seed used for random number calculations
		unsigned int adc_bits{}; //!< The number of bits to use for quantization
		unsigned int filter_length{}; //!< The length of the filter for rendering purposes
		rs_parms::BinaryFileType filetype{}; //!< The type of binary files produced by binary rendering
		bool export_xml{}; //!< Export results in XML format
		bool export_csv{}; //!< Export results in CSV format
		bool export_binary{}; //!< Export results in binary format
		unsigned int render_threads{}; //!< Number of threads to use to render each receiver
		unsigned int oversample_ratio{}; //!< Ratio of oversampling applied to pulses before rendering
	};

	/// Object which contains all the simulation parameters
	SimParameters sim_parms;
}

//Private constructor for rsParameter, should only be called once
RsParameters::RsParameters()
{
	//Default value of c, speed of light in a vacuum
	sim_parms.c = 299792458.0;
	//Simulation defaults to zero length
	sim_parms.start = 0;
	sim_parms.end = 0;
	//CW Interpolation rate defaults to 1000 per second
	sim_parms.cw_sample_rate = 1000;
	// Oversample by default
	sim_parms.rate = 0;
	// Default filter length is 33
	sim_parms.filter_length = 33;
	// Binary file type defaults to CSV
	sim_parms.filetype = rs_parms::RS_FILE_FERSBIN;
	// Don't export xml by default
	sim_parms.export_xml = false;
	// Don't export csv by default
	sim_parms.export_csv = false;
	// Export binary by default
	sim_parms.export_binary = true;
	// The random seed is set the to the current time by default
	sim_parms.random_seed = static_cast<unsigned int>(time(nullptr));
	// The default is not to quantize
	sim_parms.adc_bits = 0;
	// Default maximum number of render threads
	sim_parms.render_threads = 1;
	// Default is to disable oversampling
	sim_parms.oversample_ratio = 1;
}

RsParameters* RsParameters::modifyParms()
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return _instance;
}

//Getters for settings
rsFloat RsParameters::c()
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.c;
}

rsFloat RsParameters::boltzmannK()
{
	return 1.3806503e-23;
}

rsFloat RsParameters::startTime()  // NOLINT
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.start;
}

rsFloat RsParameters::endTime()  // NOLINT
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.end;
}

rsFloat RsParameters::cwSampleRate()
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.cw_sample_rate;
}

rs_parms::BinaryFileType RsParameters::binaryFileType()
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.filetype;
}

rsFloat RsParameters::rate()  // NOLINT
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.rate;
}

unsigned int RsParameters::randomSeed()
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.random_seed;
}

unsigned int RsParameters::adcBits()  // NOLINT
{
	if (!_instance)
	{
		_instance = new RsParameters();
	}
	return sim_parms.adc_bits;
}

bool RsParameters::exportXml()  // NOLINT
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.export_xml;
}

bool RsParameters::exportCsv()  // NOLINT
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.export_csv;
}

bool RsParameters::exportBinary()
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.export_binary;
}

/// Length to use for the rendering filter
unsigned int RsParameters::renderFilterLength()
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.filter_length;
}

/// Maximum number of threads to use for rendering
unsigned int RsParameters::renderThreads()
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.render_threads;
}

unsigned int RsParameters::oversampleRatio()
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.oversample_ratio;
}

//
//Setters for global parameters
//

void RsParameters::setC(const rsFloat c)
{
	sim_parms.c = c;
	rs_debug::printf(rs_debug::RS_CRITICAL, "[CRITICAL] Propagation speed (c) set to custom value: %8.5f\n", c);
}

void RsParameters::setTime(const rsFloat start, const rsFloat end)
{
	sim_parms.start = start;
	sim_parms.end = end;
}

void RsParameters::setCwSampleRate(const rsFloat rate)
{
	sim_parms.cw_sample_rate = rate;
}

void RsParameters::setRate(const rsFloat factor)
{
	sim_parms.rate = factor;
	rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] System sample rate set to custom value: %8.5f\n", factor);
}

void RsParameters::setRandomSeed(const unsigned int randomSeed)
{
	sim_parms.random_seed = randomSeed;
}

void RsParameters::setBinaryFileType(const rs_parms::BinaryFileType type)
{
	sim_parms.filetype = type;
}

void RsParameters::setExporters(const bool xml, const bool csv, const bool binary)
{
	sim_parms.export_xml = xml;
	sim_parms.export_csv = csv;
	sim_parms.export_binary = binary;
}

void RsParameters::setAdcBits(const unsigned int bits)
{
	sim_parms.adc_bits = bits;
}

void RsParameters::setRenderFilterLength(const unsigned int length)
{
	//Sanity check the render filter length
	if (length < 16)
	{
		throw std::runtime_error("[ERROR] Render filter length must be > 16");
	}
	sim_parms.filter_length = length;
	rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Render filter length set to custom value: %d\n", length);
}

void RsParameters::setOversampleRatio(const unsigned int ratio)
{
	//Sanity check the ratio
	if (ratio == 0)
	{
		throw std::runtime_error("[ERROR] Oversample ratio must be >= 1");
	}
	sim_parms.oversample_ratio = ratio;
	rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Oversampling enabled with ratio %d\n", ratio);
}

void RsParameters::setThreads(const unsigned int threads)
{
	sim_parms.render_threads = threads;
}
