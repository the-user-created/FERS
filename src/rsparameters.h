// rsparameters.h
// Singleton class to hold common simulation parameters
// The parameters system holds all global simulation parameters, magic numbers and other global values
// No 'magic numbers' (such as values of c) are to be used in the code - store them here instead
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 June 2006

#ifndef RS_PARAMETERS_H
#define RS_PARAMETERS_H

#include "config.h"

namespace rs_parms
{
	enum BinaryFileType { RS_FILE_CSV, RS_FILE_FERSBIN, RS_FILE_RAW };
}

namespace rs
{
	// TODO: This shouldn't be a class, it should be a namespace
	class RsParameters
	{
	public:
		static RsParameters* modifyParms();

		static RS_FLOAT c();

		static RS_FLOAT boltzmannK();

		static RS_FLOAT startTime();

		static RS_FLOAT endTime();

		static RS_FLOAT cwSampleRate();

		static RS_FLOAT rate();

		static unsigned randomSeed();

		static unsigned adcBits();

		static rs_parms::BinaryFileType binaryFileType();

		static bool exportXml();

		static bool exportCsv();

		static bool exportBinary();

		static unsigned renderFilterLength();

		static unsigned renderThreads();

		static unsigned oversampleRatio();

		static void setC(RS_FLOAT c);

		static void setTime(RS_FLOAT start, RS_FLOAT end);

		static void setCwSampleRate(RS_FLOAT rate);

		static void setRate(RS_FLOAT factor);

		static void setRandomSeed(unsigned randomSeed);

		static void setBinaryFileType(rs_parms::BinaryFileType type);

		static void setExporters(bool xml, bool csv, bool binary);

		static void setAdcBits(unsigned bits);

		static void setRenderFilterLength(unsigned length);

		static void setOversampleRatio(unsigned ratio);

		static void setThreads(unsigned threads);

	protected:
		RsParameters();

		static RsParameters* _instance;
	};
}

#endif
