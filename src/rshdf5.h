/// rshdf5.h - Header file for HDF5 export functions
/// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
/// 03 November 2006

#ifndef __RS_HDF5_H
#define __RS_HDF5_H

#include <complex>

namespace rshdf5 {

///Open the HDF5 file for writing
int CreateFile(const std::string& name);

///Add a dataset to the HDF5 file
void AddChunkToFile(int file, std::complex<rsFloat> *data, unsigned int size, rsFloat time, rsFloat rate, unsigned int count);

///Close the HDF5 file
 void CloseFile(int file);

}

#endif
