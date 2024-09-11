/// rshdf5.cpp - Export binary data to the HDF5 file format
/// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
/// 03 November 2006

#include "rshdf5.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "config.h"
#include "rsparameters.h"

using namespace rshdf5;

extern "C" {
#include <hdf5.h>
#include <H5LTpublic.h>
}

///Open the HDF5 file for reading
hid_t openFile(const std::string& name)
{
	const hid_t file = H5Fopen(name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file < 0)
	{
		throw std::runtime_error("[ERROR] Could not open HDF5 file " + name + " to read pulse");
	}
	return file;
}

/// Read the pulse data from the specified file
void rshdf5::readPulseData(const std::string& name, std::complex<RS_FLOAT>** data, unsigned int& size, RS_FLOAT& rate)
{
	rate = rs::RsParameters::rate();
	//Open the HDF5 file
	const hid_t file = openFile(name);
	//Get the size of the dataset "pulse"
	size_t type_size;
	H5T_class_t class_id;
	//Open the / group
	const hid_t slash = H5Gopen1(file, "/");
	if (slash < 0)
	{
		throw std::runtime_error("[ERROR] HDF5 file " + name + " does not have top level group \"/\"");
	}
	//Open the I group
	const hid_t i_group = H5Gopen1(slash, "I");
	if (i_group < 0)
	{
		throw std::runtime_error("[ERROR] HDF5 file " + name + " does not have group \"I\"");
	}
	// Get the rank of the groups
	int rank;
	H5LTget_dataset_ndims(i_group, "value", &rank);
	auto* dims = new hsize_t[rank];
	//Get the data set information
	herr_t res = H5LTget_dataset_info(i_group, "value", &dims[0], &class_id, &type_size);
	if (res < 0)
	{
		throw std::runtime_error("[ERROR] HDF5 file " + name + " does not have dataset \"value\" in group \"I\"");
	}
	// Allocate memory for the pulse
	size = dims[0];
	auto* buffer_i = new double[size];
	// Read in the I dataset
	res = H5LTread_dataset_double(i_group, "value", buffer_i);
	if (res < 0)
	{
		throw std::runtime_error("[ERROR] Error reading dataset I of file " + name);
	}
	//Close the I group
	H5Gclose(i_group);
	//Open the Q group
	const hid_t q_group = H5Gopen1(slash, "Q");
	if (q_group < 0)
	{
		throw std::runtime_error("[ERROR] HDF5 file " + name + " does not have group \"Q\"");
	}
	// Check the Q dataset is the same size
	res = H5LTget_dataset_info(q_group, "value", &dims[0], &class_id, &type_size);
	if (res < 0)
	{
		throw std::runtime_error("[ERROR] HDF5 file " + name + " does not have dataset \"Q\"");
	}
	if (size != dims[0])
	{
		throw std::runtime_error("[ERROR] Dataset \"Q\" is not the same size as dataset \"I\" in file " + name);
	}
	//Allocate memory for the Q set
	auto* buffer_q = new double[size];
	//Read in the Q datase
	res = H5LTread_dataset_double(q_group, "value", buffer_q);
	if (res < 0)
	{
		throw std::runtime_error("[ERROR] Error reading dataset Q of file " + name);
	}
	// Close group q
	H5Gclose(q_group);
	H5Gclose(slash);
	//Close the HDF5 file
	H5Fclose(file);
	//Copy the data to the target array
	*data = new std::complex<RS_FLOAT>[size];
	for (unsigned int i = 0; i < size; i++)
	{
		(*data)[i] = std::complex<RS_FLOAT>(buffer_i[i], buffer_q[i]);
	}
	// Clean up memory
	delete[] buffer_i;
	delete[] buffer_q;
	delete[] dims;
}

///Open the HDF5 file for writing
long int rshdf5::createFile(const std::string& name)
{
	const hid_t file = H5Fcreate(name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	if (file < 0)
	{
		throw std::runtime_error("[ERROR] Could not create HDF5 file " + name + " for export");
	}
	return file;
}

///Add a dataset to the HDF5 file
void rshdf5::addChunkToFile(const long int file, const std::complex<RS_FLOAT>* data, const unsigned int size, const RS_FLOAT time, const RS_FLOAT rate,
                            const RS_FLOAT fullscale, const unsigned int count)
{
	//Create the name of the dataset
	std::ostringstream oss;
	oss << "chunk_" << std::setw(6) << std::setfill('0') << count;
	const std::string i_chunk_name = oss.str() + "_I";
	const std::string q_chunk_name = oss.str() + "_Q";
	//Create the size variable needed by the lite api
	const hsize_t datasize = size;
	//Write out the I data
	auto* i = new double[size];
	auto* q = new double[size];
	//Seperate I and Q
	for (unsigned int it = 0; it < size; it++)
	{
		i[it] = data[it].real();
		q[it] = data[it].imag();
	}
	//Create the dataset, using the HDF5 Lite API
	if (H5LTmake_dataset_double(file, i_chunk_name.c_str(), 1, &datasize, i) < 0)
	{
		throw std::runtime_error("[ERROR] Error while writing data to HDF5 file");
	}
	if (H5LTmake_dataset_double(file, q_chunk_name.c_str(), 1, &datasize, q) < 0)
	{
		throw std::runtime_error("[ERROR] Error while writing data to HDF5 file");
	}

	//Add attributes to the data set, with the attributes of the response
	if (H5LTset_attribute_double(file, i_chunk_name.c_str(), "time", &time, 1) < 0)
	{
		throw std::runtime_error("[ERROR] Error while setting attribute \"time\" on chunk " + i_chunk_name);
	}
	if (H5LTset_attribute_double(file, i_chunk_name.c_str(), "rate", &rate, 1) < 0)
	{
		throw std::runtime_error("[ERROR] Error while setting attribute \"rate\" on chunk " + i_chunk_name);
	}
	if (H5LTset_attribute_double(file, i_chunk_name.c_str(), "fullscale", &fullscale, 1) < 0)
	{
		throw std::runtime_error("[ERROR] Error while setting attribute \"fullscale\" on chunk " + i_chunk_name);
	}
	if (H5LTset_attribute_double(file, q_chunk_name.c_str(), "time", &time, 1) < 0)
	{
		throw std::runtime_error("[ERROR] Error while setting attribute \"time\" on chunk " + q_chunk_name);
	}
	if (H5LTset_attribute_double(file, q_chunk_name.c_str(), "rate", &rate, 1) < 0)
	{
		throw std::runtime_error("[ERROR] Error while setting attribute \"rate\" on chunk " + q_chunk_name);
	}
	if (H5LTset_attribute_double(file, q_chunk_name.c_str(), "fullscale", &fullscale, 1) < 0)
	{
		throw std::runtime_error("[ERROR] Error while setting attribute \"fullscale\" on chunk " + q_chunk_name);
	}

	//Free the buffers
	delete[] i;
	delete[] q;
}

///Close the HDF5 file
void rshdf5::closeFile(const long int file)
{
	if (H5Fclose(file) < 0)
	{
		throw std::runtime_error("[ERROR] Error while closing HDF5 file");
	}
}

/// Read an antenna gain pattern or RCS pattern from a file
RS_FLOAT** rshdf5::readPattern(const std::string& name, const std::string& datasetName, unsigned int& aziSize,
                              unsigned int& elevSize)
{
	int rank;
	hsize_t dims[2];
	size_t type_size;
	H5T_class_t data_class;
	//Load the HDF5 file
	const hid_t file_id = H5Fopen(name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file_id < 0)
	{
		throw std::runtime_error("[ERROR] Cannot open HDF5 file " + name + " to read antenna data");
	}
	// Get the rank of the dataset
	herr_t err = H5LTget_dataset_ndims(file_id, datasetName.c_str(), &rank);
	if (err < 0)
	{
		throw std::runtime_error("[ERROR] Could not get rank of dataset \"" + datasetName + "\" in file " + name);
	}
	if (rank != 2)
	{
		throw std::runtime_error("[ERROR] Dataset \"" + datasetName + "\" in file " + name + " does not have rank 2");
	}
	//Get the dimensions of the file
	err = H5LTget_dataset_info(file_id, datasetName.c_str(), &dims[0], &data_class, &type_size);
	if (err < 0)
	{
		throw std::runtime_error(
			"[ERROR] Could not get dimensions of dataset \"" + datasetName + "\" in file " + name);
	}
	if (type_size != sizeof(float))
	{
		throw std::runtime_error("[ERROR] Type size incorrect in dataset \"" + datasetName + "\" in file " + name);
	}
	// Allocate memory for the pattern
	auto* data = new float[dims[0] * dims[1]];
	/// Load the pattern into memory
	err = H5LTread_dataset_float(file_id, datasetName.c_str(), data);
	if (err < 0)
	{
		delete[] data;
		throw std::runtime_error(
			"[ERROR] Could not read float data from dataset \"" + datasetName + "\" in file" + name);
	}
	///Close the HDF5 file
	if (H5Fclose(file_id) < 0)
	{
		throw std::runtime_error("[ERROR] Error while closing HDF5 file " + name);
	}
	/// Break the data down into a 2D array
	aziSize = dims[0];
	elevSize = dims[1];
	auto** ret = new RS_FLOAT*[aziSize];
	for (unsigned int i = 0; i < aziSize; i++)
	{
		ret[i] = new RS_FLOAT[elevSize];
		for (unsigned int j = 0; j < elevSize; j++)
		{
			ret[i][j] = data[i * aziSize + j];
		}
	}
	//Clean up
	delete[] data;
	return ret;
}
