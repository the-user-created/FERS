//rsparameters.h
//Singleton class to hold common simulation parameters
// The parameters system holds all global simulation parameters, magic numbers and other global values
//No 'magic numbers' (such as values of c) are to be used in the code - store them here instead
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//11 June 2006

#ifndef RS_PARAMETERS_H
#define RS_PARAMETERS_H

#include <config.h>

namespace rs_parms
{
	enum BinaryFileType { RS_FILE_CSV, RS_FILE_FERSBIN, RS_FILE_RAW };
}

namespace rs
{
	///'Singleton' class to hold parameters for the simulation
	class RsParameters
	{
	public:
		/// Method to return a pointer to the single instance of the class that can be used to modify parameters
		static RsParameters* modifyParms();

		/// Get the value of c (propagation speed in the medium)
		static rsFloat c();

		/// Get the value of boltzmann's constant
		static rsFloat boltzmannK();

		/// Get the value of start (start time of the simulation)
		static rsFloat startTime();

		/// Get the value of end (end time of the simulation)
		static rsFloat endTime();

		/// Get the CW interpolation sample rate
		static rsFloat cwSampleRate();

		/// Get the oversample factor
		static rsFloat rate();

		/// Get the current random seed
		static unsigned int randomSeed();

		/// Get the number of ADC bits used for quantization
		static unsigned int adcBits();

		/// Get the binary file type
		static rs_parms::BinaryFileType binaryFileType();

		/// Do we export in XML format?
		static bool exportXml();

		/// Do we export in CSV format?
		static bool exportCsv();

		/// Do we export in binary format?
		static bool exportBinary();

		/// Length to use for the rendering filter
		static unsigned int renderFilterLength();

		/// Maximum number of threads to use for rendering
		static unsigned int renderThreads();

		/// Number of times to oversample loaded pulses before simulation
		static unsigned int oversampleRatio();

		/// Set the value of c
		static void setC(rsFloat c);

		/// Set the start and end times
		static void setTime(rsFloat start, rsFloat end);

		/// Set the CW sample rate
		static void setCwSampleRate(rsFloat rate);

		/// Set the export sample rate
		static void setRate(rsFloat factor);

		/// Set the random seed
		static void setRandomSeed(unsigned int randomSeed);

		/// Set the binary file type
		static void setBinaryFileType(rs_parms::BinaryFileType type);

		/// Set the enabled exporters
		static void setExporters(bool xml, bool csv, bool binary);

		/// Set the number of bits used for quantization
		static void setAdcBits(unsigned int bits);

		/// Set the render filter length
		static void setRenderFilterLength(unsigned int length);

		/// Set the number of times to oversample loaded pulses before simulation
		static void setOversampleRatio(unsigned int ratio);

		/// Set the number of threads to use
		static void setThreads(unsigned int threads);

	protected:
		/// The default constructor is private
		RsParameters();

		/// Pointer to a single instance of the class
		static RsParameters* _instance;
	};
}
#endif
