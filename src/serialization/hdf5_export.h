// hdf5_export.h
// Header file for HDF5 export functions
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 03 November 2006

#ifndef HDF5_EXPORT_H
#define HDF5_EXPORT_H

#include <complex>
#include <vector>

#include "config.h"

namespace hdf5_export
{
	long createFile(const std::string& name);

	void addChunkToFile(long file, const std::complex<RS_FLOAT>* data, unsigned size, RS_FLOAT time, RS_FLOAT rate,
	                    RS_FLOAT fullscale, unsigned count);

	void closeFile(long file);

	void readPulseData(const std::string& name, std::complex<RS_FLOAT>** data, unsigned& size, RS_FLOAT& rate);

	std::vector<std::vector<RS_FLOAT>> readPattern(const std::string& name, const std::string& datasetName, unsigned& aziSize,
	                                               unsigned& elevSize);
}

#endif
