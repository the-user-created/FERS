// pulse_factory.cpp
// Created by David Young on 9/12/24.
// Original code by Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//

#include "pulse_factory.h"

#include <complex>                           // for operator>>
#include <cstddef>                           // for size_t
#include <filesystem>                        // for path
#include <fstream>                           // for char_traits, basic_istream
#include <span>                              // for span
#include <stdexcept>                         // for runtime_error
#include <string_view>                       // for string_view
#include <utility>                           // for move
#include <vector>                            // for vector

#include "config.h"                          // for RealType, ComplexType
#include "hdf5_handler.h"                    // for readPulseData
#include "core/parameters.h"
#include "signal/radar_signal.h"  // for Signal, RadarSignal

using signal::Signal;
using signal::RadarSignal;

namespace
{
	std::unique_ptr<RadarSignal> loadPulseFromHdf5File(const std::string& name, const std::filesystem::path& filepath,
	                                                   const RealType power, const RealType carrierFreq)
	{
		std::vector<ComplexType> data;
		serial::readPulseData(filepath, data);

		auto signal = std::make_unique<Signal>();
		signal->load(data, data.size(), params::rate());
		return std::make_unique<RadarSignal>(name, power, carrierFreq,
			static_cast<RealType>(data.size()) / params::rate(),std::move(signal));
	}

	std::unique_ptr<RadarSignal> loadPulseFromCsvFile(const std::string& name, const std::filesystem::path& filepath,
	                                                  const RealType power, const RealType carrierFreq)
	{
		std::ifstream ifile(filepath);
		if (!ifile)
		{
			throw std::runtime_error("Could not open file '" + filepath.string() + "' to read pulse waveform");
		}

		RealType rlength, rate;
		ifile >> rlength >> rate;

		const auto length = static_cast<std::size_t>(rlength);
		std::vector<ComplexType> data(length);

		// Read the file data
		for (std::size_t done = 0; done < length && ifile >> data[done]; ++done) {}

		if (ifile.fail() || data.size() != length)
		{
			throw std::runtime_error("Could not read full pulse waveform from file '" + filepath.string() + "'");
		}

		auto signal = std::make_unique<Signal>();
		signal->load(data, length, rate);
		return std::make_unique<RadarSignal>(name, power, carrierFreq, rlength / rate, std::move(signal));
	}

	constexpr bool hasExtension(const std::string_view filename, const std::string_view ext)
	{
		return filename.ends_with(ext);
	}
}

namespace serial
{
	std::unique_ptr<RadarSignal> loadPulseFromFile(const std::string& name, const std::string& filename,
	                                               const RealType power, const RealType carrierFreq)
	{
		const std::filesystem::path filepath = filename;
		const auto extension = filepath.extension().string();

		if (hasExtension(extension, ".csv")) { return loadPulseFromCsvFile(name, filepath, power, carrierFreq); }
		if (hasExtension(extension, ".h5")) { return loadPulseFromHdf5File(name, filepath, power, carrierFreq); }
		throw std::runtime_error("Unrecognized file extension '" + extension + "' for file: " + filename);
	}
}
