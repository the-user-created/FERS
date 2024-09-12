// rsparameters.cpp
// Implementation of Singleton class to hold common simulation parameters
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 June 2006

#include "rsparameters.h"

#include <ctime>
#include <stdexcept>

#include "logging.h"

// TODO: rsparameters needs a major refactor

using namespace rs;

RsParameters* RsParameters::_instance = new RsParameters();

namespace
{
	struct SimParameters
	{
		RS_FLOAT c{}; //!< Propagation speed of the wave in the medium
		RS_FLOAT start{}; //!< The start time of the simulation
		RS_FLOAT end{}; //!< The end time of the simulation
		RS_FLOAT cw_sample_rate{}; //<! The number of samples per second to take of changes in the CW state
		RS_FLOAT rate{}; //!< The sample rate to use for rendering
		unsigned random_seed{}; //!< The seed used for random number calculations
		unsigned adc_bits{}; //!< The number of bits to use for quantization
		unsigned filter_length{}; //!< The length of the filter for rendering purposes
		rs_parms::BinaryFileType filetype{}; //!< The type of binary files produced by binary rendering
		bool export_xml{}; //!< Export results in XML format
		bool export_csv{}; //!< Export results in CSV format
		bool export_binary{}; //!< Export results in binary format
		unsigned render_threads{}; //!< Number of threads to use to render each receiver
		unsigned oversample_ratio{}; //!< Ratio of oversampling applied to pulses before rendering
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
	sim_parms.random_seed = static_cast<unsigned>(time(nullptr));
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
RS_FLOAT RsParameters::c()
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.c;
}

RS_FLOAT RsParameters::boltzmannK()
{
	return 1.3806503e-23;
}

RS_FLOAT RsParameters::startTime() // NOLINT
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.start;
}

RS_FLOAT RsParameters::endTime() // NOLINT
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.end;
}

RS_FLOAT RsParameters::cwSampleRate()
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

RS_FLOAT RsParameters::rate() // NOLINT
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.rate;
}

unsigned RsParameters::randomSeed()
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.random_seed;
}

unsigned RsParameters::adcBits() // NOLINT
{
	if (!_instance)
	{
		_instance = new RsParameters();
	}
	return sim_parms.adc_bits;
}

bool RsParameters::exportXml() // NOLINT
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.export_xml;
}

bool RsParameters::exportCsv() // NOLINT
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
unsigned RsParameters::renderFilterLength()
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.filter_length;
}

/// Maximum number of threads to use for rendering
unsigned RsParameters::renderThreads()
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.render_threads;
}

unsigned RsParameters::oversampleRatio()
{
	if (!_instance)
	{
		_instance = new RsParameters;
	}
	return sim_parms.oversample_ratio;
}

// =====================================================================================================================
//
// GLOBAL SETTERS
//
// =====================================================================================================================

void RsParameters::setC(const RS_FLOAT c)
{
	sim_parms.c = c;
	logging::printf(logging::RS_CRITICAL, "[CRITICAL] Propagation speed (c) set to custom value: %8.5f\n", c);
}

void RsParameters::setTime(const RS_FLOAT start, const RS_FLOAT end)
{
	sim_parms.start = start;
	sim_parms.end = end;
}

void RsParameters::setCwSampleRate(const RS_FLOAT rate)
{
	sim_parms.cw_sample_rate = rate;
}

void RsParameters::setRate(const RS_FLOAT factor)
{
	sim_parms.rate = factor;
	logging::printf(logging::RS_VERY_VERBOSE, "[VV] System sample rate set to custom value: %8.5f\n", factor);
}

void RsParameters::setRandomSeed(const unsigned randomSeed)
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

void RsParameters::setAdcBits(const unsigned bits)
{
	sim_parms.adc_bits = bits;
}

void RsParameters::setRenderFilterLength(const unsigned length)
{
	if (length < 16)
	{
		throw std::runtime_error("[ERROR] Render filter length must be > 16");
	}
	sim_parms.filter_length = length;
	logging::printf(logging::RS_VERY_VERBOSE, "[VV] Render filter length set to custom value: %d\n", length);
}

void RsParameters::setOversampleRatio(const unsigned ratio)
{
	if (ratio == 0)
	{
		throw std::runtime_error("[ERROR] Oversample ratio must be >= 1");
	}
	sim_parms.oversample_ratio = ratio;
	logging::printf(logging::RS_VERY_VERBOSE, "[VV] Oversampling enabled with ratio %d\n", ratio);
}

void RsParameters::setThreads(const unsigned threads)
{
	sim_parms.render_threads = threads;
}
