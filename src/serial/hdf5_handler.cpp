// hdf5_export.cpp
// Export binary data to the HDF5 file format
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 03 November 2006

#include "hdf5_handler.h"

#include <algorithm>
#include <complex>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <highfive/H5File.hpp>

#include "core/logging.h"
#include "highfive/H5DataSet.hpp"
#include "highfive/H5DataSpace.hpp"
#include "highfive/H5Exception.hpp"
#include "highfive/H5Group.hpp"
#include "highfive/bits/H5Annotate_traits_misc.hpp"
#include "highfive/bits/H5Node_traits_misc.hpp"

using logging::Level;

namespace serial
{
	void readPulseData(const std::string& name, std::vector<ComplexType>& data)
	{
		// Check if the file exists
		if (!std::filesystem::exists(name))
		{
			LOG(Level::FATAL, "File '{}' not found", name);
			throw std::runtime_error("File " + name + " not found.");
		}

		// Open the HDF5 file in read-only mode
		LOG(Level::TRACE, "Opening file '{}'", name);
		const HighFive::File file(name, HighFive::File::ReadOnly);

		// Helper lambda to open group and read dataset
		auto read_dataset = [&file](const std::string& groupName, std::vector<double>& buffer) -> size_t
		{
			// Open the group
			const auto group = file.getGroup("/" + groupName);

			// Read the dataset named "value" into the buffer
			const auto dataset = group.getDataSet("value");

			// Get dataset dimensions using structured bindings
			const auto dimensions = dataset.getSpace().getDimensions();
			const auto size = dimensions[0];

			buffer.resize(size);
			dataset.read(buffer);

			return size;
		};

		// Read the I dataset
		LOG(Level::TRACE, "Reading dataset 'I' from file '{}'", name);
		std::vector<double> buffer_i;
		const auto size = read_dataset("I", buffer_i);

		// Read the Q dataset and ensure it has the same size as I
		std::vector<double> buffer_q;
		LOG(Level::TRACE, "Reading dataset 'Q' from file '{}'", name);
		if (read_dataset("Q", buffer_q) != size)
		{
			throw std::runtime_error(R"(Dataset "Q" is not the same size as dataset "I" in file )" + name);
		}

		// Allocate and populate the complex data using std::vector
		data.resize(size);
		for (size_t i = 0; i < size; ++i) { data[i] = ComplexType(buffer_i[i], buffer_q[i]); }
		LOG(Level::TRACE, "Read dataset successfully");
	}

	void addChunkToFile(HighFive::File& file, const std::vector<ComplexType>& data, const unsigned size,
	                    const RealType time, const RealType rate, const RealType fullscale, const unsigned count)
	{
		// Generate chunk names
		LOG(Level::TRACE, "Adding chunk {} to file", count);
		const std::string base_chunk_name = "chunk_" + std::format("{:06}", count);
		const std::string i_chunk_name = base_chunk_name + "_I";
		const std::string q_chunk_name = base_chunk_name + "_Q";

		// Prepare data for I (real) and Q (imaginary) parts using ranges
		std::vector<double> i(size), q(size);
		std::ranges::transform(data, i.begin(), [](const ComplexType& c) { return c.real(); });
		std::ranges::transform(data, q.begin(), [](const ComplexType& c) { return c.imag(); });

		// Function to write a dataset to the HDF5 file
		auto write_chunk = [&](const std::string& chunkName, const std::vector<double>& chunkData)
		{
			try
			{
				HighFive::DataSet dataset = file.createDataSet<double>(chunkName, HighFive::DataSpace::From(chunkData));
				dataset.write(chunkData);
			}
			catch (const HighFive::Exception& err)
			{
				throw std::runtime_error("Error while writing data to HDF5 file: " + chunkName + " - " + err.what());
			}
		};

		// Function to set attributes on the dataset
		auto set_chunk_attributes = [&](const std::string& chunkName)
		{
			try
			{
				HighFive::DataSet dataset = file.getDataSet(chunkName);
				dataset.createAttribute("time", time);
				dataset.createAttribute("rate", rate);
				dataset.createAttribute("fullscale", fullscale);
			}
			catch (const HighFive::Exception& err)
			{
				throw std::runtime_error("Error while setting attributes on chunk: " + chunkName + " - " + err.what());
			}
		};

		// Write the I and Q chunks and set their attributes
		LOG(Level::TRACE, "Writing chunks to file");
		write_chunk(i_chunk_name, i);
		write_chunk(q_chunk_name, q);

		set_chunk_attributes(i_chunk_name);
		set_chunk_attributes(q_chunk_name);
	}

	std::vector<std::vector<RealType>> readPattern(const std::string& name, const std::string& datasetName)
	{
		try
		{
			LOG(Level::TRACE, "Reading dataset '{}' from file '{}'", datasetName, name);
			// Open the file in read-only mode using HighFive
			const HighFive::File file(name, HighFive::File::ReadOnly);

			// Open the dataset
			const auto dataset = file.getDataSet(datasetName);

			// Get the dataset's dataspace and its dimensions
			const auto dataspace = dataset.getSpace();
			const auto dims = dataspace.getDimensions();

			// Verify the dataset dimensions
			if (dims.size() != 2)
			{
				throw std::runtime_error(std::format(R"(Invalid dataset dimensions for "{}" in file "{}")", datasetName,
				                                     name));
			}

			LOG(Level::TRACE, "Reading dataset with dimensions {}x{}", dims[0], dims[1]);

			// Read the data into a 2D vector
			std::vector data(dims[0], std::vector<RealType>(dims[1]));
			dataset.read(data);

			LOG(Level::TRACE, "Read dataset successfully");

			return data;
		}
		catch (const HighFive::Exception& err)
		{
			LOG(Level::FATAL, "Error handling HDF5 file: {}", err.what());
			throw std::runtime_error("Error handling HDF5 file: " + std::string(err.what()));
		}
	}
}
