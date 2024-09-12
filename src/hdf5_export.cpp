// hdf5_export.cpp
// Export binary data to the HDF5 file format
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 03 November 2006

#include "hdf5_export.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "config.h"
#include "rsparameters.h"

extern "C" {
#include <hdf5.h>
#include <H5LTpublic.h>
}

using namespace hdf5_export;

hid_t openFile(const std::string& name)
{
	const hid_t file = H5Fopen(name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file < 0)
	{
		throw std::runtime_error("[ERROR] Could not open HDF5 file " + name + " to read pulse");
	}
	return file;
}


void hdf5_export::readPulseData(const std::string& name, std::complex<RS_FLOAT>** data, unsigned& size, RS_FLOAT& rate)
{
	// TODO: Can be optimized and simplified
	rate = rs::RsParameters::rate();
	const hid_t file = openFile(name);
	size_t type_size;
	H5T_class_t class_id;
	const hid_t slash = H5Gopen1(file, "/");
	if (slash < 0)
	{
		throw std::runtime_error("[ERROR] HDF5 file " + name + " does not have top level group \"/\"");
	}
	const hid_t i_group = H5Gopen1(slash, "I");
	if (i_group < 0)
	{
		throw std::runtime_error("[ERROR] HDF5 file " + name + " does not have group \"I\"");
	}
	int rank;
	H5LTget_dataset_ndims(i_group, "value", &rank);
	auto* dims = new hsize_t[rank];
	herr_t res = H5LTget_dataset_info(i_group, "value", &dims[0], &class_id, &type_size);
	if (res < 0)
	{
		throw std::runtime_error("[ERROR] HDF5 file " + name + R"( does not have dataset "value" in group "I")");
	}
	size = dims[0];
	auto* buffer_i = new double[size];
	res = H5LTread_dataset_double(i_group, "value", buffer_i);
	if (res < 0)
	{
		throw std::runtime_error("[ERROR] Error reading dataset I of file " + name);
	}
	H5Gclose(i_group);
	const hid_t q_group = H5Gopen1(slash, "Q");
	if (q_group < 0)
	{
		throw std::runtime_error("[ERROR] HDF5 file " + name + " does not have group \"Q\"");
	}
	res = H5LTget_dataset_info(q_group, "value", &dims[0], &class_id, &type_size);
	if (res < 0)
	{
		throw std::runtime_error("[ERROR] HDF5 file " + name + " does not have dataset \"Q\"");
	}
	if (size != dims[0])
	{
		throw std::runtime_error(R"([ERROR] Dataset "Q" is not the same size as dataset "I" in file )" + name);
	}
	auto* buffer_q = new double[size];
	res = H5LTread_dataset_double(q_group, "value", buffer_q);
	if (res < 0)
	{
		throw std::runtime_error("[ERROR] Error reading dataset Q of file " + name);
	}
	H5Gclose(q_group);
	H5Gclose(slash);
	H5Fclose(file);
	*data = new std::complex<RS_FLOAT>[size];
	for (unsigned i = 0; i < size; i++)
	{
		(*data)[i] = std::complex<RS_FLOAT>(buffer_i[i], buffer_q[i]);
	}
	delete[] buffer_i;
	delete[] buffer_q;
	delete[] dims;
}

long hdf5_export::createFile(const std::string& name)
{
	const hid_t file = H5Fcreate(name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	if (file < 0)
	{
		throw std::runtime_error("[ERROR] Could not create HDF5 file " + name + " for export");
	}
	return file;
}

void hdf5_export::addChunkToFile(const long file, const std::complex<RS_FLOAT>* data, const unsigned size,
                                 const RS_FLOAT time, const RS_FLOAT rate, const RS_FLOAT fullscale,
                                 const unsigned count)
{
	// TODO: Can be optimized and simplified
	std::ostringstream oss;
	oss << "chunk_" << std::setw(6) << std::setfill('0') << count;
	const std::string i_chunk_name = oss.str() + "_I";
	const std::string q_chunk_name = oss.str() + "_Q";
	const hsize_t datasize = size;
	auto* i = new double[size];
	auto* q = new double[size];
	for (unsigned it = 0; it < size; it++)
	{
		i[it] = data[it].real();
		q[it] = data[it].imag();
	}
	if (H5LTmake_dataset_double(file, i_chunk_name.c_str(), 1, &datasize, i) < 0)
	{
		throw std::runtime_error("[ERROR] Error while writing data to HDF5 file");
	}
	if (H5LTmake_dataset_double(file, q_chunk_name.c_str(), 1, &datasize, q) < 0)
	{
		throw std::runtime_error("[ERROR] Error while writing data to HDF5 file");
	}

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

	delete[] i;
	delete[] q;
}

void hdf5_export::closeFile(const long file)
{
	if (H5Fclose(file) < 0)
	{
		throw std::runtime_error("[ERROR] Error while closing HDF5 file");
	}
}

RS_FLOAT** hdf5_export::readPattern(const std::string& name, const std::string& datasetName, unsigned& aziSize,
                                    unsigned& elevSize)
{
	// TODO: Can be optimized and simplified
	int rank;
	hsize_t dims[2];
	size_t type_size;
	H5T_class_t data_class;
	const hid_t file_id = H5Fopen(name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file_id < 0)
	{
		throw std::runtime_error("[ERROR] Cannot open HDF5 file " + name + " to read antenna data");
	}
	herr_t err = H5LTget_dataset_ndims(file_id, datasetName.c_str(), &rank);
	if (err < 0)
	{
		throw std::runtime_error("[ERROR] Could not get rank of dataset \"" + datasetName + "\" in file " + name);
	}
	if (rank != 2)
	{
		throw std::runtime_error("[ERROR] Dataset \"" + datasetName + "\" in file " + name + " does not have rank 2");
	}
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
	auto* data = new float[dims[0] * dims[1]];
	err = H5LTread_dataset_float(file_id, datasetName.c_str(), data);
	if (err < 0)
	{
		delete[] data;
		throw std::runtime_error(
			"[ERROR] Could not read float data from dataset \"" + datasetName + "\" in file" + name);
	}
	if (H5Fclose(file_id) < 0)
	{
		throw std::runtime_error("[ERROR] Error while closing HDF5 file " + name);
	}
	aziSize = dims[0];
	elevSize = dims[1];
	auto** ret = new RS_FLOAT*[aziSize];
	for (unsigned i = 0; i < aziSize; i++)
	{
		ret[i] = new RS_FLOAT[elevSize];
		for (unsigned j = 0; j < elevSize; j++)
		{
			ret[i][j] = data[i * aziSize + j];
		}
	}
	delete[] data;
	return ret;
}
