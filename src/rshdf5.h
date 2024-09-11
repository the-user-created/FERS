/// rshdf5.h - Header file for HDF5 export functions
/// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
/// 03 November 2006

#ifndef RS_HDF5_H
#define RS_HDF5_H

#include <complex>

#include "config.h"

namespace rshdf5
{
	///Open the HDF5 file for writing
	long int createFile(const std::string& name);

	///Add a dataset to the HDF5 file
	void addChunkToFile(long int file, const std::complex<RS_FLOAT>* data, unsigned int size, RS_FLOAT time, RS_FLOAT rate,
	                    RS_FLOAT fullscale, unsigned int count);

	///Close the HDF5 file
	void closeFile(long int file);

	/// Read the pulse data from the specified file
	void readPulseData(const std::string& name, std::complex<RS_FLOAT>** data, unsigned int& size, RS_FLOAT& rate);

	/// Read an antenna gain pattern or RCS pattern from a file
	RS_FLOAT** readPattern(const std::string& name, const std::string& datasetName, unsigned int& aziSize,
	                      unsigned int& elevSize);
}

#endif
