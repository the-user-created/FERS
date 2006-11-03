/// rshdf5.cpp - Export binary data to the HDF5 file format
/// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
/// 03 November 2006

#include <config.h>
#include <stdexcept>
#include <string>
#include <sstream>
#include "rshdf5.h"

using namespace rshdf5;

/// If we don't have HDF5, compile with stub versions of all the functions
#ifndef HAVE_LIBHDF5
/// Throw an exception if there isn't HDF5 support compiled int
void NoHDF5()
{
  throw std::logic_error("[ERROR] FERS was not compiled with HDF5 support. Cannot export in this format");
}
int rshdf5::CreateFile(const std::string& name) { NoHDF5(); }
void rshdf5::AddChunkToFile(int file, std::complex<rsFloat> data, unsigned int size, rsFloat time, rsFloat rate, unsigned int count) {  NoHDF5(); }
void rshdf5::CloseFile(int file) { NoHDF5(); }
#else //HAVE_LIBHDF5

//If we have the HDF5 library, include it's header

extern "C" {
#include <hdf5.h>
#include <H5LT.h>
}

///Open the HDF5 file for writing
int rshdf5::CreateFile(const std::string& name)
{
  hid_t file = H5Fcreate(name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  if (file < 0)
    throw std::runtime_error("[ERROR] Could not create HDF5 file "+name+" for export");
  return static_cast<int>(file);
}

///Add a dataset to the HDF5 file
void rshdf5::AddChunkToFile(int file, std::complex<rsFloat> *data, unsigned int size, rsFloat time, rsFloat rate, unsigned int count)
{
  //Create the name of the dataset
  std::ostringstream oss;
  oss << "chunk_" << count;
  std::string chunk_name = oss.str();
  //Create the size variable needed by the lite api
  hsize_t datasize = size*2;
  double *double_data = reinterpret_cast<double*>(data);
  //Create the dataset, using the HDF5 Lite API
  if (H5LTmake_dataset_double(static_cast<hid_t>(file), chunk_name.c_str(), 1, &datasize, double_data) < 0)
    throw std::runtime_error("[ERROR] Error while writing data to HDF5 file");
  //Add an attribute to the dataset, with it's start time and sample rate
  if (H5LTset_attribute_double(static_cast<hid_t>(file), chunk_name.c_str(), "time", &time, 1) < 0)
    throw std::runtime_error("[ERROR] Error while setting attribute \"time\" on chunk " + chunk_name);
  if (H5LTset_attribute_double(static_cast<hid_t>(file), chunk_name.c_str(), "rate", &rate, 1) < 0)
    throw std::runtime_error("[ERROR] Error while setting attribute \"rate\" on chunk " + chunk_name);
}

///Close the HDF5 file
void rshdf5::CloseFile(int file) {
  if (H5Fclose(static_cast<hid_t>(file)) < 0)
    throw std::runtime_error("[ERROR] Error while closing HDF5 file");
}

#endif //HAVE_LIBHDF5
