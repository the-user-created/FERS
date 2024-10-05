// hdf5_handler.h
// Header file for HDF5 export functions
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 03 November 2006

#ifndef HDF5_EXPORT_H
#define HDF5_EXPORT_H

#include <string>
#include <vector>

#include "config.h"

namespace HighFive
{
	class File;
}

namespace serial
{
	void addChunkToFile(HighFive::File& file, const std::vector<ComplexType>& data, unsigned size, RealType time,
	                    RealType rate, RealType fullscale, unsigned count);

	void readPulseData(const std::string& name, std::vector<ComplexType>& data);

	std::vector<std::vector<RealType>> readPattern(const std::string& name, const std::string& datasetName);
}

#endif
