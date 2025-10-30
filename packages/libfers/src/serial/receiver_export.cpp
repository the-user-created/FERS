// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
 * @file receiver_export.cpp
 * @brief Implementation of functions to export receiver data to various file formats.
 *
 * This file contains the logic for serializing processed receiver data into
 * standard formats such as XML, CSV, and HDF5. It acts as the final stage of the
 * data pipeline, responsible for writing simulation results to disk.
 */

#include "receiver_export.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <tuple>
#include <vector>
#include <highfive/highfive.hpp>
#include <libxml/parser.h>

#include <libfers/config.h>
#include <libfers/parameters.h>
#include <libfers/receiver.h>
#include <libfers/response.h>
#include "core/thread_pool.h"
#include "hdf5_handler.h"
#include "libxml_wrapper.h"
#include "processing/signal_processor.h"
#include "signal/dsp_filters.h"
#include "timing/timing.h"

namespace fs = std::filesystem;

namespace
{
	/**
	 * @brief Opens an HDF5 file for writing, creating it if it doesn't exist.
	 * @param recvName The base name for the HDF5 file.
	 * @return An optional `HighFive::File` object.
	 * @throws std::runtime_error If the file cannot be opened.
	 */
	std::optional<HighFive::File> openHdf5File(const std::string& recvName)
	{
		const auto hdf5_filename = std::format("{}.h5", recvName);

		try
		{
			return HighFive::File(hdf5_filename, HighFive::File::Truncate);
		}
		catch (const HighFive::Exception& err)
		{
			throw std::runtime_error("Error opening HDF5 file: " + std::string(err.what()));
		}
	}

	/**
	 * @brief Generates phase noise samples for a processing window.
	 *
	 * This function generates phase noise based on the receiver's timing model. It
	 * has the side effect of advancing the timing model's state for the next window.
	 *
	 * @param recv A pointer to the receiver object.
	 * @param wSize The size of the window in samples.
	 * @param rate The sample rate in Hz.
	 * @param carrier Output parameter for the carrier frequency from the timing model.
	 * @param enabled Output parameter indicating if phase noise is enabled.
	 * @return A vector of phase noise samples in radians.
	 */
	// NOLINTNEXTLINE(readability-non-const-parameter)
	std::vector<RealType> generatePhaseNoise(radar::Receiver* recv, const unsigned wSize, const RealType rate,
	                                         RealType& carrier, bool& enabled)
	{
		const auto timing = recv->getTiming();
		if (!timing)
		{
			LOG(logging::Level::FATAL, "Could not get receiver timing model");
			throw std::runtime_error("Could not get receiver timing model");
		}

		std::vector<RealType> noise(wSize);
		enabled = timing->isEnabled();

		if (enabled)
		{
			std::ranges::generate(noise, [&] { return timing->getNextSample(); });

			if (timing->getSyncOnPulse())
			{
				timing->reset();
				timing->skipSamples(static_cast<int>(std::floor(rate * recv->getWindowSkip())));
			}
			else
			{
				timing->skipSamples(static_cast<long>(std::floor(
					rate / recv->getWindowPrf() - rate * recv->getWindowLength())));
			}

			carrier = timing->getFrequency();
		}
		else
		{
			std::ranges::fill(noise, 0.0);
			carrier = 1.0;
		}

		return noise;
	}

	/**
	 * @brief Adds phase noise to a window of complex samples.
	 * @param noise A span of phase noise samples in radians.
	 * @param window The window of complex I/Q samples to modify.
	 */
	void addPhaseNoiseToWindow(std::span<const RealType> noise, std::span<ComplexType> window)
	{
		for (auto [n, w] : std::views::zip(noise, window))
		{
			w *= std::polar(1.0, n);
		}
	}
}

namespace serial
{
	void exportReceiverXml(const std::span<const std::unique_ptr<Response>> responses, const std::string& filename)
	{
		const XmlDocument doc;

		xmlNodePtr root_node = xmlNewNode(nullptr, reinterpret_cast<const xmlChar*>("receiver"));
		XmlElement root(root_node);

		doc.setRootElement(root);

		for (const auto& response : responses)
		{
			response->renderXml(root);
		}

		if (const fs::path file_path = fs::path(filename).replace_extension(".fersxml");
			!doc.saveFile(file_path.string()))
		{
			LOG(logging::Level::FATAL, "Failed to save XML file: {}", file_path.string());
			throw std::runtime_error("Failed to save XML file: " + file_path.string());
		}
	}

	void exportReceiverCsv(const std::span<const std::unique_ptr<Response>> responses, const std::string& filename)
	{
		std::map<std::string, std::unique_ptr<std::ofstream>> streams;

		for (const auto& response : responses)
		{
			const std::string& transmitter_name = response->getTransmitterName();

			auto [it, inserted] = streams.try_emplace(transmitter_name, nullptr);

			if (inserted)
			{
				fs::path file_path = fs::path(filename).concat("_").concat(transmitter_name).replace_extension(".csv");

				auto of = std::make_unique<std::ofstream>(file_path.string());
				of->setf(std::ios::scientific);

				if (!*of)
				{
					LOG(logging::Level::FATAL, "Could not open file {} for writing", file_path.string());
					throw std::runtime_error("Could not open file " + file_path.string() + " for writing");
				}

				it->second = std::move(of);
			}

			response->renderCsv(*it->second);
		}
	}

	void exportReceiverBinary(const std::span<const std::unique_ptr<Response>> responses, radar::Receiver* recv,
	                          const std::string& recvName, pool::ThreadPool& pool)
	{
		if (responses.empty()) { return; }

		std::optional<HighFive::File> out_bin = openHdf5File(recvName);

		const unsigned window_count = recv->getWindowCount();

		const RealType length = recv->getWindowLength();
		const RealType rate = params::rate() * params::oversampleRatio();
		const auto size = static_cast<unsigned>(std::ceil(length * rate));

		RealType carrier;
		bool pn_enabled;

		for (unsigned i = 0; i < window_count; ++i)
		{
			auto pnoise = generatePhaseNoise(recv, size, rate, carrier, pn_enabled);

			RealType start = recv->getWindowStart(i) + pnoise[0] / (2 * PI * carrier);

			// Calculate fractional delay for sub-sample alignment
			const auto [round_start, frac_delay] = [&start, rate]
			{
				RealType rounded_start = std::round(start * rate) / rate;
				RealType fractional_delay = start * rate - std::round(start * rate);
				return std::tuple{rounded_start, fractional_delay};
			}();
			start = round_start;

			std::vector<ComplexType> window(size);

			// Step 1: Add thermal noise to empty window
			processing::applyThermalNoise(window, recv->getNoiseTemperature(), recv->getRngEngine());

			// Step 2: Render raw responses into the window
			processing::renderWindow(window, length, start, frac_delay, responses, pool);

			// Step 3: Downsample if oversampling was used
			if (params::oversampleRatio() > 1) { window = std::move(fers_signal::downsample(window)); }

			// Step 4: Apply phase noise
			if (pn_enabled) { addPhaseNoiseToWindow(pnoise, window); }

			// Step 5: Quantize and scale
			const RealType fullscale = processing::quantizeAndScaleWindow(window);

			// Step 6: Write chunk to HDF5 file
			try
			{
				addChunkToFile(*out_bin, window, start, fullscale, i);
			}
			catch (const std::exception& e)
			{
				LOG(logging::Level::FATAL, "Error writing chunk to HDF5 file: {}", e.what());
				throw;
			}
		}
	}

	void exportReceiverCwBinary(const radar::Receiver* recv, const std::string& recvName)
	{
		const auto& iq_data = recv->getCwData();
		if (iq_data.empty())
		{
			LOG(logging::Level::INFO, "No CW data to export for receiver '{}'", recv->getName());
			return;
		}

		std::optional<HighFive::File> file_opt = openHdf5File(recvName);
		if (!file_opt) { return; }
		HighFive::File& file = *file_opt;

		std::vector<RealType> i_data(iq_data.size());
		std::vector<RealType> q_data(iq_data.size());

		std::ranges::transform(iq_data, i_data.begin(), [](const auto& c) { return c.real(); });
		std::ranges::transform(iq_data, q_data.begin(), [](const auto& c) { return c.imag(); });

		try
		{
			HighFive::DataSet i_dataset = file.createDataSet<RealType>("I_data", HighFive::DataSpace::From(i_data));
			i_dataset.write(i_data);

			HighFive::DataSet q_dataset = file.createDataSet<RealType>("Q_data", HighFive::DataSpace::From(q_data));
			q_dataset.write(q_data);

			// Add attributes to make the file self-describing
			file.createAttribute("sampling_rate", params::rate() * params::oversampleRatio());
			file.createAttribute("start_time", params::startTime());
			if (const auto timing = recv->getTiming())
			{
				// TODO: This is exporting the timing frequency, not the actual carrier frequency(ies)?
				file.createAttribute("reference_carrier_frequency", timing->getFrequency());
			}
		}
		catch (const HighFive::Exception& err)
		{
			LOG(logging::Level::FATAL, "Error writing CW data to HDF5 file '{}': {}", recvName, err.what());
			throw std::runtime_error("Error writing CW data to HDF5 file: " + recvName);
		}

		LOG(logging::Level::INFO, "Successfully exported CW data for receiver '{}' to '{}.h5'", recv->getName(),
		    recvName);
	}
}
