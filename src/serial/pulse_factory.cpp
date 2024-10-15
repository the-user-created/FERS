/**
 * @file pulse_factory.cpp
 * @brief Implementation for loading pulse data into RadarSignal objects.
 *
 * @authors David Young, Marc Brooker
 * @date 2024-09-12
 */

#include "pulse_factory.h"

#include <complex>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "config.h"
#include "hdf5_handler.h"
#include "core/parameters.h"
#include "signal/radar_signal.h"

using signal::Signal;
using signal::RadarSignal;

namespace
{
	/**
	 * @brief Loads a radar pulse from an HDF5 file and returns a RadarSignal object.
	 *
	 * @param name The name of the radar signal.
	 * @param filepath The path to the HDF5 file containing the pulse data.
	 * @param power The power of the radar signal in the pulse.
	 * @param carrierFreq The carrier frequency of the radar signal.
	 * @return A unique pointer to a RadarSignal object loaded with the pulse data.
	 * @throws std::runtime_error If the file cannot be opened or the file format is unrecognized.
	 */
	std::unique_ptr<RadarSignal> loadPulseFromHdf5File(const std::string& name, const std::filesystem::path& filepath,
	                                                   const RealType power, const RealType carrierFreq)
	{
		std::vector<ComplexType> data;
		serial::readPulseData(filepath, data);

		auto signal = std::make_unique<Signal>();
		signal->load(data, data.size(), params::rate());
		return std::make_unique<RadarSignal>(name, power, carrierFreq,
		                                     static_cast<RealType>(data.size()) / params::rate(), std::move(signal));
	}

	/**
	 * @brief Loads a radar pulse from a CSV file and returns a RadarSignal object.
	 *
	 * @param name The name of the radar signal.
	 * @param filepath The path to the CSV file containing the pulse data.
	 * @param power The power of the radar signal in the pulse.
	 * @param carrierFreq The carrier frequency of the radar signal.
	 * @return A unique pointer to a RadarSignal object loaded with the pulse data.
	 * @throws std::runtime_error If the file cannot be opened or the file format is unrecognized.
	 */
	std::unique_ptr<RadarSignal> loadPulseFromCsvFile(const std::string& name, const std::filesystem::path& filepath,
	                                                  const RealType power, const RealType carrierFreq)
	{
		std::ifstream ifile(filepath);
		if (!ifile)
		{
			LOG(logging::Level::FATAL, "Could not open file '{}' to read pulse waveform", filepath.string());
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
			LOG(logging::Level::FATAL, "Could not read full pulse waveform from file '{}'", filepath.string());
			throw std::runtime_error("Could not read full pulse waveform from file '" + filepath.string() + "'");
		}

		auto signal = std::make_unique<Signal>();
		signal->load(data, length, rate);
		return std::make_unique<RadarSignal>(name, power, carrierFreq, rlength / rate, std::move(signal));
	}

	/**
	 * @brief Checks if a filename has a specific extension.
	 *
	 * @param filename The filename to check.
	 * @param ext The extension to check for.
	 * @return True if the filename has the specified extension, false otherwise.
	 */
	constexpr bool hasExtension(const std::string_view filename, const std::string_view ext) noexcept
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

		LOG(logging::Level::FATAL, "Unrecognized file extension '{}' for file: '{}'", extension, filename);
		throw std::runtime_error("Unrecognized file extension '" + extension + "' for file: " + filename);
	}
}
