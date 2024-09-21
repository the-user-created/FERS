// hdf5_export.cpp
// Export binary data to the HDF5 file format
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 03 November 2006

#include "hdf5_handler.h"

#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "config.h"
#include "core/parameters.h"

extern "C"
{
#include <hdf5.h>
#include <H5LTpublic.h>
}

namespace hdf5_handler
{
	hid_t openFile(const std::string& name)
	{
		const hid_t file = H5Fopen(name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
		if (file < 0) { throw std::runtime_error("Could not open HDF5 file " + name + " to read pulse"); }
		return file;
	}


	void readPulseData(const std::string& name, std::vector<RS_COMPLEX>& data, RS_FLOAT& rate)
	{
		// Set rate from parameters
		rate = parameters::rate();

		// Open the HDF5 file
		const hid_t file = openFile(name);
		const hid_t slash = H5Gopen1(file, "/");
		if (slash < 0) { throw std::runtime_error("HDF5 file " + name + " does not have top level group \"/\""); }

		// Helper lambda to open group and read dataset
		auto read_dataset = [&](const std::string& groupName, std::vector<double>& buffer)
		{
			const hid_t group = H5Gopen1(slash, groupName.c_str());
			if (group < 0)
			{
				throw std::runtime_error("HDF5 file " + name + " does not have group \"" + groupName + "\"");
			}

			// Get dataset info
			H5T_class_t class_id;
			size_t type_size;
			hsize_t dims[1];
			if (H5LTget_dataset_info(group, "value", dims, &class_id, &type_size) < 0)
			{
				throw std::runtime_error("HDF5 file " + name + " does not have dataset \"" + groupName + "\"");
			}

			buffer.resize(dims[0]);
			if (H5LTread_dataset_double(group, "value", buffer.data()) < 0)
			{
				throw std::runtime_error("Error reading dataset " + groupName + " of file " + name);
			}

			H5Gclose(group);
			return dims[0];
		};

		// Read I dataset
		std::vector<double> buffer_i;
		const unsigned size = read_dataset("I", buffer_i);

		// Read Q dataset and ensure it has the same size as I
		std::vector<double> buffer_q;
		if (read_dataset("Q", buffer_q) != size)
		{
			throw std::runtime_error(R"(Dataset "Q" is not the same size as dataset "I" in file )" + name);
		}

		// Close HDF5 handles
		H5Gclose(slash);
		H5Fclose(file);

		// Allocate and populate the complex data using std::vector
		data.resize(size);
		for (unsigned i = 0; i < size; ++i) { data[i] = RS_COMPLEX(buffer_i[i], buffer_q[i]); }
	}

	long createFile(const std::string& name)
	{
		const hid_t file = H5Fcreate(name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
		if (file < 0) { throw std::runtime_error("Could not create HDF5 file " + name + " for export"); }
		return file;
	}

	void addChunkToFile(const long file, const std::vector<RS_COMPLEX>& data, const unsigned size,
	                    const RS_FLOAT time, const RS_FLOAT rate, const RS_FLOAT fullscale,
	                    const unsigned count)
	{
		// Generate chunk names
		std::ostringstream oss;
		oss << "chunk_" << std::setw(6) << std::setfill('0') << count;
		const std::string i_chunk_name = oss.str() + "_I";
		const std::string q_chunk_name = oss.str() + "_Q";
		const hsize_t datasize = size;

		// Allocate arrays for real and imaginary parts using std::vector (automatically manages memory)
		std::vector<double> i(size), q(size);
		for (unsigned it = 0; it < size; ++it)
		{
			i[it] = data[it].real();
			q[it] = data[it].imag();
		}

		// Write data to HDF5 file
		auto write_chunk = [&](const std::string& chunkName, const std::vector<double>& chunkData)
		{
			if (H5LTmake_dataset_double(file, chunkName.c_str(), 1, &datasize, chunkData.data()) < 0)
			{
				throw std::runtime_error("Error while writing data to HDF5 file: " + chunkName);
			}
		};

		// Set attributes for chunk
		auto set_chunk_attributes = [&](const std::string& chunkName)
		{
			const std::vector attributes = {time, rate, fullscale};
			const std::vector<std::string> attr_names = {"time", "rate", "fullscale"};

			for (int it = 0; it < 3; ++it)
			{
				if (H5LTset_attribute_double(file, chunkName.c_str(), attr_names[it].c_str(), &attributes[it], 1) < 0)
				{
					throw std::runtime_error(
						"Error while setting attribute \"" + std::string(attr_names[it]) + "\" on chunk " +
						chunkName);
				}
			}
		};

		// Write the I and Q chunks
		write_chunk(i_chunk_name, i);
		write_chunk(q_chunk_name, q);

		// Set attributes for both I and Q chunks
		set_chunk_attributes(i_chunk_name);
		set_chunk_attributes(q_chunk_name);
	}

	void closeFile(const long file)
	{
		if (H5Fclose(file) < 0) { throw std::runtime_error("Error while closing HDF5 file"); }
	}

	std::vector<std::vector<RS_FLOAT>> readPattern(const std::string& name, const std::string& datasetName,
	                                               unsigned& aziSize,
	                                               unsigned& elevSize)
	{
		hsize_t dims[2];
		const hid_t file_id = H5Fopen(name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
		if (file_id < 0) { throw std::runtime_error("Cannot open HDF5 file " + name + " to read antenna data"); }

		int rank;
		size_t type_size;
		H5T_class_t data_class;
		if (H5LTget_dataset_ndims(file_id, datasetName.c_str(), &rank) < 0 || rank != 2 ||
			H5LTget_dataset_info(file_id, datasetName.c_str(), dims, &data_class, &type_size) < 0 || type_size != sizeof
			(
				float))
		{
			H5Fclose(file_id);
			throw std::runtime_error("Invalid dataset \"" + datasetName + "\" in file " + name);
		}

		std::vector<float> data(dims[0] * dims[1]);
		if (H5LTread_dataset_float(file_id, datasetName.c_str(), data.data()) < 0)
		{
			H5Fclose(file_id);
			throw std::runtime_error(
				"Could not read float data from dataset \"" + datasetName + "\" in file " + name);
		}

		if (H5Fclose(file_id) < 0) { throw std::runtime_error("Error while closing HDF5 file " + name); }

		aziSize = dims[0];
		elevSize = dims[1];

		std::vector ret(aziSize, std::vector<RS_FLOAT>(elevSize));
		for (unsigned i = 0; i < aziSize; ++i)
		{
			for (unsigned j = 0; j < elevSize; ++j) { ret[i][j] = data[i * elevSize + j]; }
		}
		return ret;
	}
}
