// parameters.h
// Struct to hold common simulation parameters
// The SimParameters struct stores all global simulation parameters, constants, and configuration values
// No 'magic numbers' (such as the speed of light) are to be used directly in the code - store them here instead
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 11 June 2006

#pragma once

#include <chrono>
#include <optional>
#include <stdexcept>

#include "config.h"
#include "logging.h"

namespace params
{
	/**
	 * @brief Struct to hold simulation parameters.
	 */
	struct Parameters
	{
		constexpr static RealType DEFAULT_C = 299792458.0; ///< Speed of light (m/s)
		constexpr static RealType DEFAULT_BOLTZMANN_K = 1.3806503e-23; ///< Boltzmann constant
		//constexpr static unsigned MIN_FILTER_LENGTH = 16; ///< Minimum render filter length

		RealType c = DEFAULT_C; ///< Speed of light
		RealType boltzmann_k = DEFAULT_BOLTZMANN_K; ///< Boltzmann constant
		RealType start = 0; ///< Start time for simulation
		RealType end = 0; ///< End time for simulation
		RealType cw_sample_rate = 1000; ///< CW interpolation rate
		RealType rate = 0; ///< Sample rate for rendering

		unsigned random_seed = std::chrono::system_clock::now().time_since_epoch().count(); ///< Random seed using current time
		unsigned adc_bits = 0; ///< ADC quantization bits
		unsigned filter_length = 33; ///< Default filter length

		bool export_xml = false; ///< Export XML flag
		bool export_csv = false; ///< Export CSV flag
		bool export_binary = true; ///< Export binary flag

		unsigned render_threads = 1; ///< Number of rendering threads
		unsigned oversample_ratio = 1; ///< Oversampling ratio

		std::optional<RealType> optional_rate = std::nullopt; ///< Optional sample rate
	};

	/// Global instance of SimParameters
	inline Parameters params;

	// =================================================================================================================
	//
	// GETTERS
	//
	// =================================================================================================================

	/**
	 * @brief Get the speed of light.
	 * @return Speed of light.
	 */
	inline RealType c() noexcept { return params.c; }

	/**
	 * @brief Get the Boltzmann constant.
	 * @return Boltzmann constant.
	 */
	inline RealType boltzmannK() noexcept { return params.boltzmann_k; }

	/**
	 * @brief Get the start time for simulation.
	 * @return Start time.
	 */
	inline RealType startTime() noexcept { return params.start; }

	/**
	 * @brief Get the end time for simulation.
	 * @return End time.
	 */
	inline RealType endTime() noexcept { return params.end; }

	/**
	 * @brief Get the CW interpolation rate.
	 * @return CW interpolation rate.
	 */
	inline RealType cwSampleRate() noexcept { return params.cw_sample_rate; }

	/**
	 * @brief Get the sample rate for rendering.
	 * @return Sample rate.
	 */
	inline RealType rate() noexcept { return params.rate; }

	/**
	 * @brief Get the random seed.
	 * @return Random seed.
	 */
	inline unsigned randomSeed() noexcept { return params.random_seed; }

	/**
	 * @brief Get the ADC quantization bits.
	 * @return ADC quantization bits.
	 */
	inline unsigned adcBits() noexcept { return params.adc_bits; }

	/**
	 * @brief Check if XML export is enabled.
	 * @return True if XML export is enabled, false otherwise.
	 */
	inline bool exportXml() noexcept { return params.export_xml; }

	/**
	 * @brief Check if CSV export is enabled.
	 * @return True if CSV export is enabled, false otherwise.
	 */
	inline bool exportCsv() noexcept { return params.export_csv; }

	/**
	 * @brief Check if binary export is enabled.
	 * @return True if binary export is enabled, false otherwise.
	 */
	inline bool exportBinary() noexcept { return params.export_binary; }

	/**
	 * @brief Get the render filter length.
	 * @return Render filter length.
	 */
	inline unsigned renderFilterLength() noexcept { return params.filter_length; }

	/**
	 * @brief Get the number of rendering threads.
	 * @return Number of rendering threads.
	 */
	inline unsigned renderThreads() noexcept { return params.render_threads; }

	/**
	 * @brief Get the oversampling ratio.
	 * @return Oversampling ratio.
	 */
	inline unsigned oversampleRatio() noexcept { return params.oversample_ratio; }

	// =================================================================================================================
	//
	// SETTERS (can also be accessed directly via sim_parms)
	//
	// =================================================================================================================

	/**
	 * @brief Set the speed of light.
	 * @param cValue Speed of light.
	 */
	inline void setC(RealType cValue) noexcept
	{
		params.c = cValue;
		LOG(logging::Level::INFO, "Propagation speed (c) set to: {:.5f}", cValue);
	}

	/**
	 * @brief Set the start and end time for simulation.
	 * @param startTime Start time.
	 * @param endTime End time.
	 */
	inline void setTime(const RealType startTime, const RealType endTime) noexcept
	{
		params.start = startTime;
		params.end = endTime;
		LOG(logging::Level::INFO, "Simulation time set from {:.5f} to {:.5f} seconds", startTime, endTime);
	}

	/**
	 * @brief Set the CW interpolation rate.
	 * @param rate CW interpolation rate.
	 */
	inline void setCwSampleRate(const RealType rate) noexcept
	{
		params.cw_sample_rate = rate;
		LOG(logging::Level::DEBUG, "CW interpolation rate set to: {:.5f} Hz", rate);
	}

	/**
	 * @brief Set the sample rate for rendering.
	 * @param rateValue Sample rate.
	 */
	inline void setRate(RealType rateValue) noexcept
	{
		params.rate = rateValue;
		LOG(logging::Level::DEBUG, "Sample rate set to: {:.5f}", rateValue);
	}

	/**
	 * @brief Set the random seed.
	 * @param seed Random seed.
	 */
	inline void setRandomSeed(const unsigned seed) noexcept
	{
		params.random_seed = seed;
		LOG(logging::Level::DEBUG, "Random seed set to: {}", seed);
	}

	/**
	 * @brief Set the export flags for XML, CSV, and binary.
	 * @param xml Export XML flag.
	 * @param csv Export CSV flag.
	 * @param binary Export binary flag.
	 */
	inline void setExporters(const bool xml, const bool csv, const bool binary) noexcept
	{
		params.export_xml = xml;
		params.export_csv = csv;
		params.export_binary = binary;
		LOG(logging::Level::DEBUG, "Export flags set - XML: {}, CSV: {}, Binary: {}", xml, csv, binary);
	}

	/**
	 * @brief Set the ADC quantization bits.
	 * @param bits ADC quantization bits.
	 */
	inline void setAdcBits(const unsigned bits) noexcept
	{
		params.adc_bits = bits;
		LOG(logging::Level::DEBUG, "ADC quantization bits set to: {}", bits);
	}

	/*inline void setRenderFilterLength(unsigned length)
	{
		if (length < Parameters::MIN_FILTER_LENGTH) { throw std::runtime_error("Render filter length must be >= 16"); }
		params.filter_length = length;
		LOG(logging::Level::DEBUG, "Render filter length set to: {}", length);
	}*/

	/**
	 * @brief Set the oversampling ratio.
	 * @param ratio Oversampling ratio.
	 * @throws std::runtime_error if ratio is zero.
	 */
	inline void setOversampleRatio(unsigned ratio)
	{
		if (ratio == 0) { throw std::runtime_error("Oversample ratio must be >= 1"); }
		params.oversample_ratio = ratio;
		LOG(logging::Level::DEBUG, "Oversampling enabled with ratio: {}", ratio);
	}

	/**
	 * @brief Set the number of rendering threads.
	 * @param threads Number of rendering threads.
	 */
	inline void setThreads(const unsigned threads) noexcept
	{
		params.render_threads = threads;
		LOG(logging::Level::INFO, "Number of rendering threads set to: {}", threads);
	}
}
