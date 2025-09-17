// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

// Turn FERS HDF5 output into a raw binary file with 32 bit interleaved float samples
// This was used for integeration with the G2 SAR processor, and probably isn't a good idea in most cases - the HDF5 version is much easier to work with

#include <cstdio>        // for size_t, FILE, fclose, fopen, fwrite
#include <H5LTpublic.h>  // for H5LTget_attribute_double, H5LTfind_dataset
#include <iomanip>       // for operator<<, setfill, setw
#include <iostream>      // for basic_ostream, operator<<, endl, cerr, cout
#include <sstream>       // for basic_ostringstream
#include <stdexcept>     // for runtime_error
#include <string>        // for char_traits, allocator, operator+, string

#include "H5Gpublic.h"   // for H5Gopen1
#include "H5Ipublic.h"   // for hid_t
#include "H5Ppublic.h"   // for H5Fclose, H5Fopen, H5F_ACC_RDONLY, H5P_DEFAULT
#include "H5public.h"    // for hsize_t
#include "H5Tpublic.h"   // for H5T_class_t

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


/// Read the HDF5 datasets and dump them out the the correct format
void readAndDump(hid_t file, FILE* outfile)
{
	size_t size = 0;
	unsigned it;
	double max_i = 0, max_q = 0;
	//Open the / group
	hid_t slash = H5Gopen1(file, "/");
	if (slash < 0)
	{
		throw std::runtime_error("[ERROR] HDF5 file does not have top level group \"/\"");
	}
	// Count the chunks
	size_t count = 0;
	for (it = 0; it < 100000; it++)
	{
		std::ostringstream oss;
		oss << "chunk_" << std::setw(6) << std::setfill('0') << it;
		std::string i_chunk_name = oss.str() + "_I";
		std::string q_chunk_name = oss.str() + "_Q";
		if (H5LTfind_dataset(slash, i_chunk_name.c_str()) < 1)
		{
			break;
		}
		double i_scale, q_scale;
		H5LTget_attribute_double(slash, i_chunk_name.c_str(), "fullscale", &i_scale);
		H5LTget_attribute_double(slash, q_chunk_name.c_str(), "fullscale", &q_scale);
		if (i_scale > max_i)
		{
			max_i = i_scale;
		}
		if (q_scale > max_q)
		{
			max_q = q_scale;
		}
		if (it == 0)
		{
			hsize_t* dims;
			// Get the dataset ranks
			int rank;
			if (H5LTget_dataset_ndims(slash, i_chunk_name.c_str(), &rank) < 0)
			{
				break;
			}
			dims = new hsize_t[rank];
			// Get the dataset info
			size_t type_size;
			H5T_class_t class_id;
			if (H5LTget_dataset_info(slash, i_chunk_name.c_str(), &dims[0], &class_id, &type_size) < 0)
			{
				std::cerr << "Could not get dataset info for " << i_chunk_name << std::endl;
				break;
			}
			// Allocate memory for the I and Q data
			size = dims[0];
		}
	}
	std::cout << "MaxI " << max_i << " maxQ " << max_q << std::endl;
	count = it;
	// Allocate memory for the reshuffle
	auto* buffer = new double[count * size * 2];
	for (it = 0; it < count; it++)
	{
		// Get the dataset names
		std::ostringstream oss;
		oss << "chunk_" << std::setw(6) << std::setfill('0') << it;
		std::string i_chunk_name = oss.str() + "_I";
		std::string q_chunk_name = oss.str() + "_Q";
		if (H5LTfind_dataset(slash, i_chunk_name.c_str()) < 1)
		{
			break;
		}
		auto* buffer_i = new double[size];
		auto* buffer_q = new double[size];
		H5LTread_dataset_double(slash, i_chunk_name.c_str(), buffer_i);
		H5LTread_dataset_double(slash, q_chunk_name.c_str(), buffer_q);
		// Get the fullscale for I and Q
		double i_scale, q_scale;
		H5LTget_attribute_double(slash, i_chunk_name.c_str(), "fullscale", &i_scale);
		H5LTget_attribute_double(slash, q_chunk_name.c_str(), "fullscale", &q_scale);
		// Write out the data
		for (unsigned j = 0; j < size; j++)
		{
			double i = buffer_i[j] * i_scale / max_i;
			double q = buffer_q[j] * q_scale / max_q;

			buffer[(j + it * size) * 2] = i;
			buffer[(j + it * size) * 2 + 1] = q;
		}
		//Clean up
		delete[] buffer_i;
		delete[] buffer_q;
	}
	fwrite(buffer, count * size * 2, sizeof(float), outfile);
	std::cout << "Read " << it - 1 << " windows of length " << size << std::endl;
	delete[] buffer;
}

int main(const int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: h52raw infile outfile\n";
		return 1;
	}
	const hid_t infile = openFile(argv[1]);

	FILE* outfile = fopen(argv[2], "w");
	if (!outfile)
	{
		std::cerr << "Could not open file " << argv[2] << std::endl;
		return 1;
	}
	//Perform the reading and dumping
	readAndDump(infile, outfile);
	//Close the files
	H5Fclose(infile);
	fclose(outfile);

	return 0;
}
