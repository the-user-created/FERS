/**
* @file hdf5_handler.h
* @brief Header file for HDF5 data export and import functions.
*
* This file provides the function declarations for handling HDF5 file operations,
* including reading pulse data, adding chunks of data to HDF5 files, and reading
* pattern data from the HDF5 format.
*
* The functions facilitate data storage and retrieval in scientific or signal processing
* contexts using the HDF5 format. It leverages the HighFive library for file operations.
*
* @authors David Young, Marc Brooker
* @date 2006-11-03
*/

#pragma once

#include <string>
#include <vector>

#include "config.h"

namespace HighFive
{
	class File;
}

namespace serial
{
	/**
	* @brief Adds a chunk of data to an HDF5 file.
	*
	* This function adds a chunk of complex data to an HDF5 file. The chunk contains
	* real (I) and imaginary (Q) parts, along with attributes such as time, rate,
	* and full scale. The data is stored in a structured manner with sequential
	* chunk naming conventions.
	*
	* @param file The HDF5 file where the chunk is written.
	* @param data A vector of complex data to be written.
	* @param size The size of the data chunk.
	* @param time The time attribute associated with the chunk.
	* @param rate The rate attribute associated with the chunk.
	* @param fullscale The fullscale attribute for the chunk.
	* @param count The sequential count number for chunk naming.
	* @throws std::runtime_error If there is an error writing data or setting attributes.
	*/
	void addChunkToFile(HighFive::File& file, const std::vector<ComplexType>& data, unsigned size, RealType time,
	                    RealType rate, RealType fullscale, unsigned count);

	/**
	* @brief Reads pulse data from an HDF5 file.
	*
	* This function reads complex pulse data (I and Q datasets) from an HDF5 file.
	* It verifies the existence of the file, reads the "I" and "Q" datasets, and
	* combines them into a vector of complex numbers.
	*
	* @param name The name of the HDF5 file.
	* @param data A reference to a vector where the complex data will be stored.
	* @throws std::runtime_error If the file does not exist or the datasets "I" and "Q" have mismatched sizes.
	*/
	void readPulseData(const std::string& name, std::vector<ComplexType>& data);

	/**
	* @brief Reads a 2D pattern dataset from an HDF5 file.
	*
	* This function reads a dataset from an HDF5 file that contains a 2D pattern
	* (matrix). The dataset must have exactly two dimensions.
	*
	* @param name The name of the HDF5 file.
	* @param datasetName The name of the dataset to be read.
	* @return A 2D vector containing the pattern data.
	* @throws std::runtime_error If there is an error handling the file or if the dataset dimensions are invalid.
	*/
	std::vector<std::vector<RealType>> readPattern(const std::string& name, const std::string& datasetName);
}
