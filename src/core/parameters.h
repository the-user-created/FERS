// parameters.h
// Struct to hold common simulation parameters
// The SimParameters struct stores all global simulation parameters, constants, and configuration values
// No 'magic numbers' (such as the speed of light) are to be used directly in the code - store them here instead
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 June 2006

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <ctime>
#include <stdexcept>

#include "config.h"
#include "logging.h"

namespace parameters
{
	enum BinaryFileType { RS_FILE_CSV, RS_FILE_FERSBIN, RS_FILE_RAW };

	struct Parameters
	{
		RS_FLOAT c = 299792458.0; // Speed of light in a vacuum
		RS_FLOAT boltzmann_k = 1.3806503e-23; // Boltzmann's constant
		RS_FLOAT start = 0; // Simulation start time
		RS_FLOAT end = 0; // Simulation end time
		RS_FLOAT cw_sample_rate = 1000; // CW interpolation rate
		RS_FLOAT rate = 0; // Sample rate for rendering
		unsigned random_seed = static_cast<unsigned>(time(nullptr)); // Random seed initialized to current time
		unsigned adc_bits = 0; // ADC quantization bits
		unsigned filter_length = 33; // Render filter length
		BinaryFileType filetype = RS_FILE_FERSBIN; // Binary file type
		bool export_xml = false; // Export XML format
		bool export_csv = false; // Export CSV format
		bool export_binary = true; // Export a binary format
		unsigned render_threads = 1; // Number of rendering threads
		unsigned oversample_ratio = 1; // Oversampling ratio
	};

	// Global instance of SimParameters
	extern Parameters params;

	// =================================================================================================================
	//
	// GETTERS
	//
	// =================================================================================================================

	inline RS_FLOAT c() { return params.c; }

	inline RS_FLOAT boltzmannK() { return params.boltzmann_k; }

	inline RS_FLOAT startTime() { return params.start; }

	inline RS_FLOAT endTime() { return params.end; }

	inline RS_FLOAT cwSampleRate() { return params.cw_sample_rate; }

	inline RS_FLOAT rate() { return params.rate; }

	inline unsigned randomSeed() { return params.random_seed; }

	inline unsigned adcBits() { return params.adc_bits; }

	inline BinaryFileType binaryFileType() { return params.filetype; }

	inline bool exportXml() { return params.export_xml; }

	inline bool exportCsv() { return params.export_csv; }

	inline bool exportBinary() { return params.export_binary; }

	inline unsigned renderFilterLength() { return params.filter_length; }

	inline unsigned renderThreads() { return params.render_threads; }

	inline unsigned oversampleRatio() { return params.oversample_ratio; }

	// =================================================================================================================
	//
	// SETTERS (can also be accessed directly via sim_parms)
	//
	// =================================================================================================================

	// NOTE: These SETTERS are not thread-safe, but since they are only set by
	//		 main.cpp and xmlimport.cpp, this is not a concern for now

	inline void setC(const RS_FLOAT c)
	{
		params.c = c;
		logging::printf(logging::RS_CRITICAL, "[CRITICAL] Propagation speed (c) set to custom value: %8.5f\n", c);
	}

	inline void setTime(const RS_FLOAT start, const RS_FLOAT end)
	{
		params.start = start;
		params.end = end;
	}

	inline void setCwSampleRate(const RS_FLOAT rate) { params.cw_sample_rate = rate; }

	inline void setRate(const RS_FLOAT rate)
	{
		params.rate = rate;
		logging::printf(logging::RS_VERY_VERBOSE, "[VV] System sample rate set to custom value: %8.5f\n", rate);
	}

	inline void setRandomSeed(const unsigned seed) { params.random_seed = seed; }

	inline void setBinaryFileType(const BinaryFileType type)
	{
		// Note: This function is not used in the codebase
		params.filetype = type;
	}

	inline void setExporters(const bool xml, const bool csv, const bool binary)
	{
		params.export_xml = xml;
		params.export_csv = csv;
		params.export_binary = binary;
	}

	inline void setAdcBits(const unsigned bits) { params.adc_bits = bits; }

	inline void setRenderFilterLength(const unsigned length)
	{
		// Note: This function is not used in the codebase
		if (length < 16) { throw std::runtime_error("[ERROR] Render filter length must be > 16"); }
		params.filter_length = length;
		logging::printf(logging::RS_VERY_VERBOSE, "[VV] Render filter length set to custom value: %d\n", length);
	}

	inline void setOversampleRatio(const unsigned ratio)
	{
		if (ratio == 0) { throw std::runtime_error("[ERROR] Oversample ratio must be >= 1"); }
		params.oversample_ratio = ratio;
		logging::printf(logging::RS_VERY_VERBOSE, "[VV] Oversampling enabled with ratio %d\n", ratio);
	}

	inline void setThreads(const unsigned threads) { params.render_threads = threads; }
}

#endif
