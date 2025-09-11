/**
* @file parameters.h
* @brief Defines the Parameters struct and provides methods for managing simulation parameters.
*
* @authors David Young, Marc Brooker
* @date 2006-06-11
*/

#pragma once

#include <chrono>
#include <optional>
#include <string>

#include "config.h"
#include "logging.h"

namespace params
{
	/**
	 * @enum CoordinateFrame
	 * @brief Defines the coordinate systems supported for scenario definition.
	 */
	enum class CoordinateFrame
	{
		ENU, ///< East-North-Up local tangent plane (default)
		UTM, ///< Universal Transverse Mercator
		ECEF ///< Earth-Centered, Earth-Fixed
	};

	/**
	* @class Parameters
	* @brief Struct to hold simulation parameters.
	*/
	struct Parameters
	{
		constexpr static RealType DEFAULT_C = 299792458.0; ///< Speed of light (m/s)
		constexpr static RealType DEFAULT_BOLTZMANN_K = 1.3806503e-23; ///< Boltzmann constant
		RealType c = DEFAULT_C; ///< Speed of light (modifiable)
		RealType boltzmann_k = DEFAULT_BOLTZMANN_K; ///< Boltzmann constant (modifiable)
		RealType start = 0; ///< Start time for the simulation.
		RealType end = 0; ///< End time for the simulation.
		RealType sim_sampling_rate = 1000;

		///< Temporal sampling rate (Hz) that determines time-step resolution for radar pulse simulation.
		// Default to the location of the University of Cape Town in South Africa
		double origin_latitude = -33.957652; ///< Geodetic origin latitude
		double origin_longitude = 18.4611991; ///< Geodetic origin longitude
		double origin_altitude = 111.01; ///< Geodetic origin altitude (in meters)
		CoordinateFrame coordinate_frame = CoordinateFrame::ENU; ///< Scenario coordinate frame
		int utm_zone = 0; ///< UTM zone (1-60), if applicable
		bool utm_north_hemisphere = true; ///< UTM hemisphere, if applicable
		RealType rate = 0; ///< Rendering sample rate.
		std::optional<unsigned> random_seed; ///< Random seed for simulation.
		unsigned adc_bits = 0; ///< ADC quantization bits.
		unsigned filter_length = 33; ///< Default render filter length.
		bool export_xml = false; ///< Enable or disable XML export.
		bool export_csv = false; ///< Enable or disable CSV export.
		bool export_binary = true; ///< Enable or disable binary export.
		unsigned render_threads = 1; ///< Number of rendering threads to use.
		std::string simulation_name; ///< The name of the simulation, from the XML.
		unsigned oversample_ratio = 1; ///< Oversampling ratio.
	};

	inline Parameters params;

	/**
	* @brief Get the speed of light.
	* @return The speed of light in meters per second.
	*/
	inline RealType c() noexcept { return params.c; }

	/**
	* @brief Get the Boltzmann constant.
	* @return The Boltzmann constant.
	*/
	inline RealType boltzmannK() noexcept { return params.boltzmann_k; }

	/**
	* @brief Get the start time for the simulation.
	* @return Start time for the simulation.
	*/
	inline RealType startTime() noexcept { return params.start; }

	/**
	* @brief Get the end time for the simulation.
	* @return End time for the simulation.
	*/
	inline RealType endTime() noexcept { return params.end; }

	/**
	* @brief Get the simulation sampling rate.
	* @return The simulation sampling rate.
	*/
	inline RealType simSamplingRate() noexcept { return params.sim_sampling_rate; }

	/**
	* @brief Get the rendering sample rate.
	* @return The rendering sample rate.
	*/
	inline RealType rate() noexcept { return params.rate; }

	/**
	* @brief Get the random seed.
	* @return The current random seed value.
	*/
	inline unsigned randomSeed() noexcept { return params.random_seed.value_or(0); }

	/**
	* @brief Get the ADC quantization bits.
	* @return Number of ADC quantization bits.
	*/
	inline unsigned adcBits() noexcept { return params.adc_bits; }

	/**
	* @brief Check if XML export is enabled.
	* @return True if XML export is enabled, otherwise false.
	*/
	inline bool exportXml() noexcept { return params.export_xml; }

	/**
	* @brief Check if CSV export is enabled.
	* @return True if CSV export is enabled, otherwise false.
	*/
	inline bool exportCsv() noexcept { return params.export_csv; }

	/**
	* @brief Check if binary export is enabled.
	* @return True if binary export is enabled, otherwise false.
	*/
	inline bool exportBinary() noexcept { return params.export_binary; }

	/**
	* @brief Get the render filter length.
	* @return The length of the render filter.
	*/
	inline unsigned renderFilterLength() noexcept { return params.filter_length; }

	/**
	* @brief Get the number of rendering threads.
	* @return The number of rendering threads.
	*/
	inline unsigned renderThreads() noexcept { return params.render_threads; }

	/**
	* @brief Get the oversampling ratio.
	* @return The oversampling ratio.
	*/
	inline unsigned oversampleRatio() noexcept { return params.oversample_ratio; }

	/**
	* @brief Set the speed of light.
	* @param cValue The new speed of light.
	*/
	inline void setC(RealType cValue) noexcept
	{
		params.c = cValue;
		LOG(logging::Level::INFO, "Propagation speed (c) set to: {:.5f}", cValue);
	}

	/**
	* @brief Set the start and end times for the simulation.
	* @param startTime Start time for the simulation.
	* @param endTime End time for the simulation.
	*/
	inline void setTime(const RealType startTime, const RealType endTime) noexcept
	{
		params.start = startTime;
		params.end = endTime;
		LOG(logging::Level::INFO, "Simulation time set from {:.5f} to {:.5f} seconds", startTime, endTime);
	}

	/**
	* @brief Set the simulation sampling rate.
	* @param rate The new simulation sampling rate.
	*/
	inline void setSimSamplingRate(const RealType rate) noexcept
	{
		params.sim_sampling_rate = rate;
		LOG(logging::Level::DEBUG, "Simulation sampling rate set to: {:.5f} Hz", rate);
	}

	/**
	* @brief Set the rendering sample rate.
	* @param rateValue The new sample rate for rendering.
	*/
	inline void setRate(RealType rateValue)
	{
		if (rateValue <= 0) { throw std::runtime_error("Sampling rate must be > 0"); }
		params.rate = rateValue;
		LOG(logging::Level::DEBUG, "Sample rate set to: {:.5f}", rateValue);
	}

	/**
	* @brief Set the random seed.
	* @param seed The new random seed value.
	*/
	inline void setRandomSeed(const unsigned seed) noexcept
	{
		params.random_seed = seed;
		LOG(logging::Level::DEBUG, "Random seed set to: {}", seed);
	}

	/**
	* @brief Set the export options for XML, CSV, and binary formats.
	* @param xml Enable or disable XML export.
	* @param csv Enable or disable CSV export.
	* @param binary Enable or disable binary export.
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
	* @param bits The new ADC quantization bits.
	*/
	inline void setAdcBits(const unsigned bits) noexcept
	{
		params.adc_bits = bits;
		LOG(logging::Level::DEBUG, "ADC quantization bits set to: {}", bits);
	}

	/**
	* @brief Set the oversampling ratio.
	* @param ratio The new oversampling ratio.
	* @throws std::runtime_error if the ratio is zero.
	*/
	inline void setOversampleRatio(unsigned ratio)
	{
		if (ratio == 0) { throw std::runtime_error("Oversample ratio must be >= 1"); }
		params.oversample_ratio = ratio;
		LOG(logging::Level::DEBUG, "Oversampling enabled with ratio: {}", ratio);
	}

	/**
	* @brief Set the geodetic origin for the KML generator.
	* @param lat The latitude of the origin.
	* @param lon The longitude of the origin.
	* @param alt The altitude of the origin (MSL).
	*/
	inline void setOrigin(const double lat, const double lon, const double alt) noexcept
	{
		params.origin_latitude = lat;
		params.origin_longitude = lon;
		params.origin_altitude = alt;
		LOG(logging::Level::INFO, "Origin set to lat: {}, lon: {}, alt: {}", lat, lon, alt);
	}

	inline double originLatitude() noexcept { return params.origin_latitude; }

	inline double originLongitude() noexcept { return params.origin_longitude; }

	inline double originAltitude() noexcept { return params.origin_altitude; }

	/**
	* @brief Set the number of rendering threads.
	* @param threads The number of rendering threads.
	* @return A `std::expected<void, std::string>` indicating success or an error message if the number of threads is invalid.
	*/
	inline std::expected<void, std::string> setThreads(const unsigned threads) noexcept
	{
		if (threads == 0) { return std::unexpected("Thread count must be >= 1"); }
		params.render_threads = threads;
		LOG(logging::Level::INFO, "Number of rendering threads set to: {}", threads);
		return {};
	}

	/**
	* @brief Set the coordinate system for the scenario.
	* @param frame The coordinate frame (ENU, UTM, ECEF).
	* @param zone The UTM zone, if applicable.
	* @param north The UTM hemisphere (true for North), if applicable.
	*/
	inline void setCoordinateSystem(const CoordinateFrame frame, const int zone, const bool north) noexcept
	{
		params.coordinate_frame = frame;
		params.utm_zone = zone;
		params.utm_north_hemisphere = north;
	}

	inline CoordinateFrame coordinateFrame() noexcept { return params.coordinate_frame; }

	inline int utmZone() noexcept { return params.utm_zone; }

	inline bool utmNorthHemisphere() noexcept { return params.utm_north_hemisphere; }
}
