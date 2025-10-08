// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
* @file xml_parser.cpp
* @brief Implementation file for parsing XML configuration files for simulation.
*/

#include "xml_parser.h"

#include <cmath>
#include <filesystem>
#include <functional>
#include <memory>
#include <random>
#include <span>
#include <string_view>
#include <utility>
#include <vector>
#include <GeographicLib/UTMUPS.hpp>

// Generated headers
#include "fers_xml_dtd.h"
#include "fers_xml_xsd.h"

// Public libfers headers
#include <libfers/antenna_factory.h>
#include <libfers/config.h>
#include <libfers/coord.h>
#include <libfers/geometry_ops.h>
#include <libfers/logging.h>
#include <libfers/parameters.h>
#include <libfers/path.h>
#include <libfers/platform.h>
#include <libfers/radar_obj.h>
#include <libfers/receiver.h>
#include <libfers/rotation_path.h>
#include <libfers/target.h>
#include <libfers/transmitter.h>
#include <libfers/world.h>

// Private libfers headers
#include "libxml_wrapper.h"
#include "pulse_factory.h"
#include "timing/prototype_timing.h"
#include "timing/timing.h"

namespace fs = std::filesystem;

using logging::Level;
using core::World;
using antenna::Antenna;
using radar::Platform;
using math::Path;
using math::RotationPath;
using radar::Target;
using fers_signal::RadarSignal;
using timing::Timing;
using timing::PrototypeTiming;
using radar::Transmitter;
using radar::Receiver;

/**
 * @brief Parses elements with child iteration (e.g., pulses, timings, antennas).
 *
 * @tparam T The type of the parsing function.
 * @param root The root XmlElement to parse.
 * @param elementName The name of the child elements to iterate over.
 * @param world A pointer to the World object where parsed data is added.
 * @param parseFunction The parsing function to call for each child element.
 */
template <typename T>
void parseElements(const XmlElement& root, const std::string& elementName, World* world, T parseFunction)
{
	unsigned index = 0;
	while (true)
	{
		XmlElement element = root.childElement(elementName, index++);
		if (!element.isValid()) { break; }
		parseFunction(element, world);
	}
}

/**
 * @brief Helper function to extract a RealType value from an element.
 *
 * @param element The XmlElement to extract the value from.
 * @param elementName The name of the child element to extract the value from.
 * @return The extracted RealType value.
 * @throws XmlException if the element is empty or cannot be parsed.
 */
auto get_child_real_type = [](const XmlElement& element, const std::string& elementName) -> RealType
{
	const std::string text = element.childElement(elementName, 0).getText();
	if (text.empty()) { throw XmlException("Element " + elementName + " is empty!"); }
	return std::stod(text);
};

/**
 * @brief Helper function to extract a boolean value from an attribute.
 *
 * @param element The XmlElement to extract the value from.
 * @param attributeName The name of the attribute to extract the value from.
 * @param defaultVal The default value to return if the attribute is empty or cannot be parsed.
 * @return The extracted boolean value or the default value.
 */
auto get_attribute_bool = [](const XmlElement& element, const std::string& attributeName, const bool defaultVal) -> bool
{
	try { return XmlElement::getSafeAttribute(element, attributeName) == "true"; }
	catch (const XmlException&)
	{
		LOG(Level::WARNING, "Failed to get boolean value for attribute '{}'. Defaulting to {}.", attributeName,
		    defaultVal);
		return defaultVal;
	}
};

namespace
{
	enum class SimType { UNSET, PULSED, CW };

	SimType simulation_type = SimType::UNSET;

	/**
	 * @brief Parses the <parameters> element of the XML document.
	 *
	 * @param parameters The <parameters> XmlElement to parse.
	 */
	void parseParameters(const XmlElement& parameters)
	{
		params::setTime(get_child_real_type(parameters, "starttime"), get_child_real_type(parameters, "endtime"));

		params::setRate(get_child_real_type(parameters, "rate"));

		auto set_param_with_exception_handling = [](const XmlElement& element, const std::string& paramName,
		                                            const RealType defaultValue,
		                                            const std::function<void(RealType)>& setter)
		{
			try
			{
				if (paramName == "adc_bits" || paramName == "oversample")
				{
					setter(static_cast<unsigned>(std::floor(get_child_real_type(element, paramName))));
				}
				else { setter(get_child_real_type(element, paramName)); }
			}
			catch (const XmlException&)
			{
				LOG(Level::WARNING, "Failed to set parameter {}. Using default value. {}", paramName, defaultValue);
			}
		};

		set_param_with_exception_handling(parameters, "c", params::c(), params::setC);
		set_param_with_exception_handling(parameters, "simSamplingRate", params::simSamplingRate(),
		                                  params::setSimSamplingRate);

		try
		{
			const auto seed = static_cast<unsigned>(std::floor(get_child_real_type(parameters, "randomseed")));
			params::params.random_seed = seed;
		}
		catch (const XmlException&)
		{
			// Do nothing, optional remains empty
		}

		set_param_with_exception_handling(parameters, "adc_bits", params::adcBits(), params::setAdcBits);
		set_param_with_exception_handling(parameters, "oversample", params::oversampleRatio(),
		                                  params::setOversampleRatio);

		if (const XmlElement export_element = parameters.childElement("export", 0); export_element.isValid())
		{
			params::setExporters(
				get_attribute_bool(export_element, "xml", params::exportXml()),
				get_attribute_bool(export_element, "csv", params::exportCsv()),
				get_attribute_bool(export_element, "binary", params::exportBinary())
				);
		}

		// Parse the origin element for the KML generator
		bool origin_set = false;
		if (const XmlElement origin_element = parameters.childElement("origin", 0); origin_element.isValid())
		{
			try
			{
				const double latitude = std::stod(XmlElement::getSafeAttribute(origin_element, "latitude"));
				const double longitude = std::stod(XmlElement::getSafeAttribute(origin_element, "longitude"));
				const double altitude = std::stod(XmlElement::getSafeAttribute(origin_element, "altitude"));
				params::setOrigin(latitude, longitude, altitude);
				origin_set = true;
			}
			catch (const std::exception& e)
			{
				LOG(Level::WARNING, "Could not parse origin from XML, using defaults. Error: {}", e.what());
			}
		}

		// Parse coordinate system, defaulting to ENU
		if (const XmlElement cs_element = parameters.childElement("coordinatesystem", 0); cs_element.isValid())
		{
			try
			{
				const std::string frame_str = XmlElement::getSafeAttribute(cs_element, "frame");
				params::CoordinateFrame frame;
				int zone = 0;
				bool north = true;

				if (frame_str == "UTM")
				{
					frame = params::CoordinateFrame::UTM;
					zone = std::stoi(XmlElement::getSafeAttribute(cs_element, "zone"));
					const std::string hem_str = XmlElement::getSafeAttribute(cs_element, "hemisphere");

					if (zone < GeographicLib::UTMUPS::MINUTMZONE || zone > GeographicLib::UTMUPS::MAXUTMZONE)
					{
						throw XmlException("UTM zone " + std::to_string(zone) + " is invalid; must be in [1, 60].");
					}
					if (hem_str == "N" || hem_str == "n") { north = true; }
					else if (hem_str == "S" || hem_str == "s") { north = false; }
					else
					{
						throw XmlException("UTM hemisphere '" + hem_str + "' is invalid; must be 'N' or 'S'.");
					}
					LOG(Level::INFO, "Coordinate system set to UTM, zone {}{}", zone, north ? 'N' : 'S');
				}
				else if (frame_str == "ECEF")
				{
					frame = params::CoordinateFrame::ECEF;
					LOG(Level::INFO, "Coordinate system set to ECEF.");
				}
				else if (frame_str == "ENU")
				{
					frame = params::CoordinateFrame::ENU;
					if (!origin_set)
					{
						LOG(Level::WARNING,
						    "ENU frame specified but no <origin> tag found. Using default origin at UCT.");
					}
					LOG(Level::INFO, "Coordinate system set to ENU local tangent plane.");
				}
				else { throw XmlException("Unsupported coordinate frame: " + frame_str); }
				params::setCoordinateSystem(frame, zone, north);
			}
			catch (const std::exception& e)
			{
				LOG(Level::WARNING, "Could not parse <coordinatesystem> from XML: {}. Defaulting to ENU.", e.what());
				params::setCoordinateSystem(params::CoordinateFrame::ENU, 0, true);
			}
		}
	}

	/**
	 * @brief Parses the <pulse> element of the XML document.
	 *
	 * @param pulse The <pulse> XmlElement to parse.
	 * @param world A pointer to the World object where the RadarSignal object is added.
	 * @param baseDir The base directory of the main scenario file to resolve relative paths.
	 */
	void parsePulse(const XmlElement& pulse, World* world, const fs::path& baseDir)
	{
		const std::string name = XmlElement::getSafeAttribute(pulse, "name");
		const std::string type = XmlElement::getSafeAttribute(pulse, "type");

		if (const XmlElement power_element = pulse.childElement("power", 0); !power_element.isValid())
		{
			LOG(Level::FATAL, "<power> element is missing in <pulse>!");
			throw XmlException("<power> element is missing in <pulse>!");
		}

		const auto power = get_child_real_type(pulse, "power");

		if (const XmlElement carrier_element = pulse.childElement("carrier", 0); !carrier_element.isValid())
		{
			LOG(Level::FATAL, "<carrier> element is missing in <pulse>!");
			throw XmlException("<carrier> element is missing in <pulse>!");
		}

		const auto carrier = get_child_real_type(pulse, "carrier");

		if (type == "continuous" || type == "cw")
		{
			auto cw_signal = std::make_unique<fers_signal::CwSignal>();
			auto wave = std::make_unique<RadarSignal>(name, power, carrier, params::endTime() - params::startTime(),
			                                          std::move(cw_signal));
			world->add(std::move(wave));
		}
		else if (type == "file")
		{
			const std::string filename_str = XmlElement::getSafeAttribute(pulse, "filename");
			fs::path pulse_path(filename_str);

			// Check if path exists as is, if not, try relative to the main XML directory
			if (!fs::exists(pulse_path))
			{
				pulse_path = baseDir / filename_str;
			}

			if (!fs::exists(pulse_path))
			{
				throw XmlException("Pulse file not found: " + filename_str);
			}

			auto wave = serial::loadPulseFromFile(name, pulse_path.string(), power, carrier);
			world->add(std::move(wave));
		}
		else
		{
			LOG(Level::FATAL, "Unsupported pulse type: {}", type);
			throw XmlException("Unsupported pulse type: " + type);
		}
	}

	/**
	 * @brief Parses the <timing> element of the XML document.
	 *
	 * @param timing The <timing> XmlElement to parse.
	 * @param world A pointer to the World object where the PrototypeTiming object is added.
	 */
	void parseTiming(const XmlElement& timing, World* world)
	{
		const std::string name = XmlElement::getSafeAttribute(timing, "name");
		const RealType freq = get_child_real_type(timing, "frequency");
		auto timing_obj = std::make_unique<PrototypeTiming>(name);

		timing_obj->setFrequency(freq);

		unsigned noise_index = 0;
		while (true)
		{
			XmlElement noise_element = timing.childElement("noise_entry", noise_index++);
			if (!noise_element.isValid()) { break; }

			timing_obj->setAlpha(
				get_child_real_type(noise_element, "alpha"),
				get_child_real_type(noise_element, "weight")
				);
		}

		try { timing_obj->setFreqOffset(get_child_real_type(timing, "freq_offset")); }
		catch (XmlException&) { LOG(Level::WARNING, "Clock section '{}' does not specify frequency offset.", name); }

		try { timing_obj->setRandomFreqOffsetStdev(get_child_real_type(timing, "random_freq_offset_stdev")); }
		catch (XmlException&)
		{
			LOG(Level::WARNING, "Clock section '{}' does not specify random frequency offset.", name);
		}

		try { timing_obj->setPhaseOffset(get_child_real_type(timing, "phase_offset")); }
		catch (XmlException&) { LOG(Level::WARNING, "Clock section '{}' does not specify phase offset.", name); }

		try { timing_obj->setRandomPhaseOffsetStdev(get_child_real_type(timing, "random_phase_offset_stdev")); }
		catch (XmlException&) { LOG(Level::WARNING, "Clock section '{}' does not specify random phase offset.", name); }

		if (get_attribute_bool(timing, "synconpulse", false)) { timing_obj->setSyncOnPulse(); }

		world->add(std::move(timing_obj));
	}

	/**
	 * @brief Parses the <antenna> element of the XML document.
	 *
	 * @param antenna The <antenna> XmlElement to parse.
	 * @param world A pointer to the World object where the Antenna object is added.
	 */
	void parseAntenna(const XmlElement& antenna, World* world)
	{
		std::string name = XmlElement::getSafeAttribute(antenna, "name");
		const std::string pattern = XmlElement::getSafeAttribute(antenna, "pattern");

		std::unique_ptr<Antenna> ant;

		LOG(Level::DEBUG, "Adding antenna '{}' with pattern '{}'", name, pattern);
		if (pattern == "isotropic") { ant = std::make_unique<antenna::Isotropic>(name); }
		else if (pattern == "sinc")
		{
			ant = std::make_unique<antenna::Sinc>(name,
			                                      get_child_real_type(antenna, "alpha"),
			                                      get_child_real_type(antenna, "beta"),
			                                      get_child_real_type(antenna, "gamma")
				);
		}
		else if (pattern == "gaussian")
		{
			ant = std::make_unique<antenna::Gaussian>(name,
			                                          get_child_real_type(antenna, "azscale"),
			                                          get_child_real_type(antenna, "elscale")
				);
		}
		else if (pattern == "squarehorn")
		{
			ant = std::make_unique<antenna::SquareHorn>(name,
			                                            get_child_real_type(antenna, "diameter")
				);
		}
		else if (pattern == "parabolic")
		{
			ant = std::make_unique<antenna::Parabolic>(name, get_child_real_type(antenna, "diameter")
				);
		}
		else if (pattern == "xml")
		{
			ant = std::make_unique<antenna::XmlAntenna>(name, XmlElement::getSafeAttribute(antenna, "filename"));
		}
		else if (pattern == "file")
		{
			ant = std::make_unique<antenna::H5Antenna>(name, XmlElement::getSafeAttribute(antenna, "filename"));
		}
		else
		{
			LOG(Level::FATAL, "Unsupported antenna pattern: {}", pattern);
			throw XmlException("Unsupported antenna pattern: " + pattern);
		}

		try { ant->setEfficiencyFactor(get_child_real_type(antenna, "efficiency")); }
		catch (XmlException&)
		{
			LOG(Level::WARNING, "Antenna '{}' does not specify efficiency, assuming unity.", name);
		}

		world->add(std::move(ant));
	}

	/**
	 * @brief Parses the <motionpath> element of the XML document.
	 *
	 * @param motionPath The <motionpath> XmlElement to parse.
	 * @param platform A pointer to the Platform object where the motion path is set.
	 */
	void parseMotionPath(const XmlElement& motionPath, const Platform* platform)
	{
		Path* path = platform->getMotionPath();
		try
		{
			if (std::string interp = XmlElement::getSafeAttribute(motionPath, "interpolation"); interp == "linear")
			{
				path->setInterp(Path::InterpType::INTERP_LINEAR);
			}
			else if (interp == "cubic") { path->setInterp(Path::InterpType::INTERP_CUBIC); }
			else if (interp == "static") { path->setInterp(Path::InterpType::INTERP_STATIC); }
			else
			{
				LOG(Level::ERROR, "Unsupported interpolation type: {} for platform {}. Defaulting to static", interp,
				    platform->getName());
				path->setInterp(Path::InterpType::INTERP_STATIC);
			}
		}
		catch (XmlException&)
		{
			LOG(Level::ERROR, "Failed to set MotionPath interpolation type for platform {}. Defaulting to static",
			    platform->getName());
			path->setInterp(Path::InterpType::INTERP_STATIC);
		}

		unsigned waypoint_index = 0;
		while (true)
		{
			XmlElement waypoint = motionPath.childElement("positionwaypoint", waypoint_index);
			if (!waypoint.isValid()) { break; }

			try
			{
				math::Coord coord;
				coord.t = get_child_real_type(waypoint, "time");
				coord.pos = math::Vec3(
					get_child_real_type(waypoint, "x"),
					get_child_real_type(waypoint, "y"),
					get_child_real_type(waypoint, "altitude")
					);
				path->addCoord(coord);
				LOG(Level::TRACE, "Added waypoint {} to motion path for platform {}.", waypoint_index,
				    platform->getName());
			}
			catch (const XmlException& e)
			{
				LOG(Level::ERROR, "Failed to add waypoint to motion path. Discarding waypoint. {}", e.what());
			}

			waypoint_index++;
		}

		path->finalize();
	}

	/**
	 * @brief Parses the <rotationpath> element of the XML document.
	 *
	 * @param rotation The <rotationpath> XmlElement to parse.
	 * @param platform A pointer to the Platform object where the rotation path is set.
	 */
	void parseRotationPath(const XmlElement& rotation, const Platform* platform)
	{
		RotationPath* path = platform->getRotationPath();

		try
		{
			if (const std::string interp = XmlElement::getSafeAttribute(rotation, "interpolation"); interp == "linear")
			{
				path->setInterp(RotationPath::InterpType::INTERP_LINEAR);
			}
			else if (interp == "cubic") { path->setInterp(RotationPath::InterpType::INTERP_CUBIC); }
			else if (interp == "static") { path->setInterp(RotationPath::InterpType::INTERP_STATIC); }
			else { throw XmlException("Unsupported interpolation type: " + interp); }
		}
		catch (XmlException&)
		{
			LOG(Level::ERROR, "Failed to set RotationPath interpolation type for platform {}. Defaulting to static",
			    platform->getName());
			path->setInterp(RotationPath::InterpType::INTERP_STATIC);
		}

		unsigned waypoint_index = 0;
		while (true)
		{
			XmlElement waypoint = rotation.childElement("rotationwaypoint", waypoint_index);
			if (!waypoint.isValid()) { break; }

			try
			{
				LOG(Level::TRACE, "Adding waypoint {} to rotation path for platform {}.", waypoint_index,
				    platform->getName());

				const RealType az_deg = get_child_real_type(waypoint, "azimuth");
				const RealType el_deg = get_child_real_type(waypoint, "elevation");
				const RealType time = get_child_real_type(waypoint, "time");

				// Convert from compass heading (degrees, CW from North) to FERS mathematical angle (radians, CCW from East)
				const RealType az_rad = (90.0 - az_deg) * (PI / 180.0);
				// Convert elevation from degrees to radians
				const RealType el_rad = el_deg * (PI / 180.0);

				path->addCoord({az_rad, el_rad, time});
			}
			catch (const XmlException& e)
			{
				LOG(Level::ERROR, "Failed to add waypoint to rotation path. Discarding waypoint. {}", e.what());
			}

			waypoint_index++;
		}

		path->finalize();
	}

	/**
	 * @brief Parses the <fixedrotation> element of the XML document.
	 *
	 * @param rotation The <fixedrotation> XmlElement to parse.
	 * @param platform A pointer to the Platform object where the fixed rotation is set.
	 */
	void parseFixedRotation(const XmlElement& rotation, const Platform* platform)
	{
		RotationPath* path = platform->getRotationPath();
		try
		{
			math::RotationCoord start, rate;
			const RealType start_az_deg = get_child_real_type(rotation, "startazimuth");
			const RealType start_el_deg = get_child_real_type(rotation, "startelevation");
			const RealType rate_az_deg_s = get_child_real_type(rotation, "azimuthrate");
			const RealType rate_el_deg_s = get_child_real_type(rotation, "elevationrate");

			// Convert compass azimuth (degrees, CW from North) to FERS mathematical angle (radians, CCW from East)
			start.azimuth = (90.0 - start_az_deg) * (PI / 180.0);
			// Convert elevation from degrees to radians
			start.elevation = start_el_deg * (PI / 180.0);

			// Convert angular rates from deg/s to rad/s.
			// A positive (CW) azimuth rate in degrees becomes a negative (CCW) rate in radians.
			rate.azimuth = -rate_az_deg_s * (PI / 180.0);
			rate.elevation = rate_el_deg_s * (PI / 180.0);

			path->setConstantRate(start, rate);
			LOG(Level::DEBUG, "Added fixed rotation to platform {}", platform->getName());
		}
		catch (XmlException& e)
		{
			LOG(Level::FATAL, "Failed to set fixed rotation for platform {}. {}", platform->getName(), e.what());
			throw XmlException("Failed to set fixed rotation for platform " + platform->getName());
		}
	}

	/**
	 * @brief Parses the <transmitter> element of the XML document.
	 *
	 * @param transmitter The <transmitter> XmlElement to parse.
	 * @param platform A pointer to the Platform
	 * @param world A pointer to the World
	 * @param masterSeeder The master random number generator for seeding.
	 * @return A pointer to the created Transmitter object.
	 */
	Transmitter* parseTransmitter(const XmlElement& transmitter, Platform* platform, World* world,
	                              std::mt19937& masterSeeder)
	{
		const std::string name = XmlElement::getSafeAttribute(transmitter, "name");
		const std::string type = XmlElement::getSafeAttribute(transmitter, "type");
		const bool is_pulsed = type == "pulsed";

		if (simulation_type == SimType::UNSET)
		{
			simulation_type = is_pulsed ? SimType::PULSED : SimType::CW;
			LOG(Level::INFO, "Simulation type set to: {}", is_pulsed ? "PULSED" : "CW");
		}
		else if ((is_pulsed && simulation_type == SimType::CW) ||
			(!is_pulsed && simulation_type == SimType::PULSED))
		{
			throw XmlException("Cannot mix pulsed and CW transmitters in the same simulation!");
		}

		if (!is_pulsed) { params::params.is_cw_simulation = true; }

		auto transmitter_obj = std::make_unique<Transmitter>(platform, name, is_pulsed);

		const std::string pulse_name = XmlElement::getSafeAttribute(transmitter, "pulse");
		RadarSignal* pulse = world->findSignal(pulse_name);
		transmitter_obj->setWave(pulse);

		transmitter_obj->setPrf(get_child_real_type(transmitter, "prf"));

		const std::string antenna_name = XmlElement::getSafeAttribute(transmitter, "antenna");
		const Antenna* ant = world->findAntenna(antenna_name);
		transmitter_obj->setAntenna(ant);

		const std::string timing_name = XmlElement::getSafeAttribute(transmitter, "timing");
		const auto timing = std::make_shared<Timing>(timing_name, masterSeeder());
		const PrototypeTiming* proto = world->findTiming(timing_name);
		timing->initializeModel(proto);
		transmitter_obj->setTiming(timing);

		world->add(std::move(transmitter_obj));

		return world->getTransmitters().back().get();
	}

	/**
	 * @brief Parses the <receiver> element of the XML document.
	 *
	 * @param receiver The <receiver> XmlElement to parse.
	 * @param platform A pointer to the Platform
	 * @param world A pointer to the World
	 * @param masterSeeder The master random number generator for seeding.
	 * @return A pointer to the created Receiver object.
	 */
	Receiver* parseReceiver(const XmlElement& receiver, Platform* platform, World* world, std::mt19937& masterSeeder)
	{
		const std::string name = XmlElement::getSafeAttribute(receiver, "name");

		auto receiver_obj = std::make_unique<Receiver>(platform, name, masterSeeder());

		const std::string ant_name = XmlElement::getSafeAttribute(receiver, "antenna");

		const Antenna* antenna = world->findAntenna(ant_name);
		receiver_obj->setAntenna(antenna);

		try { receiver_obj->setNoiseTemperature(get_child_real_type(receiver, "noise_temp")); }
		catch (XmlException&)
		{
			LOG(Level::INFO, "Receiver '{}' does not specify noise temperature",
			    receiver_obj->getName().c_str());
		}

		// Enforce required receiver parameters.
		const RealType window_length = get_child_real_type(receiver, "window_length");
		if (window_length <= 0)
		{
			throw XmlException("<window_length> must be positive for receiver '" + name + "'");
		}

		const RealType prf = get_child_real_type(receiver, "prf");
		if (prf <= 0)
		{
			throw XmlException("<prf> must be positive for receiver '" + name + "'");
		}

		const RealType window_skip = get_child_real_type(receiver, "window_skip");
		if (window_skip < 0)
		{
			throw XmlException("<window_skip> must not be negative for receiver '" + name + "'");
		}
		receiver_obj->setWindowProperties(window_length, prf, window_skip);

		const std::string timing_name = XmlElement::getSafeAttribute(receiver, "timing");
		const auto timing = std::make_shared<Timing>(timing_name, masterSeeder());

		const PrototypeTiming* proto = world->findTiming(timing_name);
		timing->initializeModel(proto);
		receiver_obj->setTiming(timing);

		if (get_attribute_bool(receiver, "nodirect", false))
		{
			receiver_obj->setFlag(Receiver::RecvFlag::FLAG_NODIRECT);
			LOG(Level::DEBUG, "Ignoring direct signals for receiver '{}'",
			    receiver_obj->getName().c_str());
		}

		if (get_attribute_bool(receiver, "nopropagationloss", false))
		{
			receiver_obj->setFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS);
			LOG(Level::DEBUG, "Ignoring propagation losses for receiver '{}'",
			    receiver_obj->getName().c_str());
		}

		world->add(std::move(receiver_obj));
		return world->getReceivers().back().get();
	}

	/**
	 * @brief Parses the <monostatic> element of the XML document.
	 *
	 * @param monostatic The <monostatic> XmlElement to parse.
	 * @param platform A pointer to the Platform
	 * @param world A pointer to the World
	 * @param masterSeeder The master random number generator for seeding.
	 */
	void parseMonostatic(const XmlElement& monostatic, Platform* platform, World* world, std::mt19937& masterSeeder)
	{
		Transmitter* trans = parseTransmitter(monostatic, platform, world, masterSeeder);
		Receiver* recv = parseReceiver(monostatic, platform, world, masterSeeder);
		trans->setAttached(recv);
		recv->setAttached(trans);
	}

	/**
	 * @brief Parses the <target> element of the XML document.
	 *
	 * @param target The <target> XmlElement to parse.
	 * @param platform A pointer to the Platform
	 * @param world A pointer to the World
	 * @param masterSeeder The master random number generator for seeding.
	 * @throws XmlException if the target element is missing required attributes or elements.
	 */
	void parseTarget(const XmlElement& target, Platform* platform, World* world, std::mt19937& masterSeeder)
	{
		const std::string name = XmlElement::getSafeAttribute(target, "name");

		const XmlElement rcs_element = target.childElement("rcs", 0);
		if (!rcs_element.isValid()) { throw XmlException("<rcs> element is required in <target>!"); }

		const std::string rcs_type = XmlElement::getSafeAttribute(rcs_element, "type");

		std::unique_ptr<Target> target_obj;
		const unsigned seed = masterSeeder();

		if (rcs_type == "isotropic")
		{
			target_obj = createIsoTarget(platform, name, get_child_real_type(rcs_element, "value"), seed);
		}
		else if (rcs_type == "file")
		{
			target_obj = createFileTarget(platform, name, XmlElement::getSafeAttribute(rcs_element, "filename"),
			                              seed);
		}
		else { throw XmlException("Unsupported RCS type: " + rcs_type); }

		if (const XmlElement model = target.childElement("model", 0); model.isValid())
		{
			if (const std::string model_type = XmlElement::getSafeAttribute(model, "type"); model_type == "constant")
			{
				target_obj->setFluctuationModel(std::make_unique<radar::RcsConst>());
			}
			else if (model_type == "chisquare" || model_type == "gamma")
			{
				target_obj->setFluctuationModel(std::make_unique<radar::RcsChiSquare>(
					target_obj->getRngEngine(),
					get_child_real_type(model, "k")
					));
			}
			else { throw XmlException("Unsupported model type: " + model_type); }
		}

		LOG(Level::DEBUG, "Added target {} with RCS type {} to platform {}", name, rcs_type, platform->getName());

		world->add(std::move(target_obj));
	}

	void parsePlatformElements(const XmlElement& platform, World* world, Platform* plat,
	                           std::mt19937& masterSeeder)
	{
		XmlElement monostatic = platform.childElement("monostatic", 0);
		XmlElement transmitter = platform.childElement("transmitter", 0);
		XmlElement receiver = platform.childElement("receiver", 0);
		XmlElement target = platform.childElement("target", 0);

		int component_count = 0;
		if (monostatic.isValid()) { component_count++; }
		if (transmitter.isValid()) { component_count++; }
		if (receiver.isValid()) { component_count++; }
		if (target.isValid()) { component_count++; }

		if (component_count != 1)
		{
			const std::string platform_name = XmlElement::getSafeAttribute(platform, "name");
			throw XmlException("Platform '" + platform_name +
				"' must contain exactly one component: <monostatic>, <transmitter>, <receiver>, or <target>.");
		}

		if (monostatic.isValid()) { parseMonostatic(monostatic, plat, world, masterSeeder); }
		else if (transmitter.isValid()) { parseTransmitter(transmitter, plat, world, masterSeeder); }
		else if (receiver.isValid()) { parseReceiver(receiver, plat, world, masterSeeder); }
		else
			if (target.isValid()) { parseTarget(target, plat, world, masterSeeder); }
	}

	/**
	 * @brief Parses the <platform> element of the XML document.
	 *
	 * @param platform The <platform> XmlElement to parse.
	 * @param world A pointer to the World object where the Platform object is added.
	 * @param masterSeeder The master random number generator for seeding.
	 */
	void parsePlatform(const XmlElement& platform, World* world, std::mt19937& masterSeeder)
	{
		std::string name = XmlElement::getSafeAttribute(platform, "name");
		auto plat = std::make_unique<Platform>(name);

		parsePlatformElements(platform, world, plat.get(), masterSeeder);

		if (const XmlElement motion_path = platform.childElement("motionpath", 0); motion_path.isValid())
		{
			parseMotionPath(motion_path, plat.get());
		}

		// Parse either <rotationpath> or <fixedrotation>
		const XmlElement rot_path = platform.childElement("rotationpath", 0);

		if (const XmlElement fixed_rot = platform.childElement("fixedrotation", 0); rot_path.isValid() && fixed_rot.
			isValid())
		{
			LOG(Level::ERROR,
			    "Both <rotationpath> and <fixedrotation> are declared for platform {}. Only <rotationpath> will be used.",
			    plat->getName());
			parseRotationPath(rot_path, plat.get());
		}
		else if (rot_path.isValid()) { parseRotationPath(rot_path, plat.get()); }
		else
			if (fixed_rot.isValid()) { parseFixedRotation(fixed_rot, plat.get()); }

		world->add(std::move(plat));
	}

	/**
	 * @brief Collects all "include" elements from the XML document and included documents.
	 *
	 * @param doc The XmlDocument to collect "include" elements from.
	 * @param currentDir The current directory of the main XML file.
	 * @param includePaths A vector to store the full paths to the included files.
	 */
	void collectIncludeElements(const XmlDocument& doc, const fs::path& currentDir, std::vector<fs::path>& includePaths)
	{
		unsigned index = 0;
		while (true)
		{
			XmlElement include_element = doc.getRootElement().childElement("include", index++);
			if (!include_element.isValid()) { break; }

			std::string include_filename = include_element.getText();
			if (include_filename.empty())
			{
				LOG(Level::ERROR, "<include> element is missing the filename!");
				continue;
			}

			// Construct the full path to the included file
			fs::path include_path = currentDir / include_filename;
			includePaths.push_back(include_path);

			XmlDocument included_doc;
			if (!included_doc.loadFile(include_path.string()))
			{
				LOG(Level::ERROR, "Failed to load included XML file: {}", include_path.string());
				continue;
			}

			// Recursively collect include elements from the included document
			collectIncludeElements(included_doc, include_path.parent_path(), includePaths);
		}
	}

	/**
	 * @brief Merges the contents of all included documents into the main document.
	 *
	 * @param mainDoc The main XmlDocument to merge the included documents into.
	 * @param currentDir The current directory of the main XML file.
	 * @return True if any included documents were merged into the main document, false otherwise.
	 */
	bool addIncludeFilesToMainDocument(const XmlDocument& mainDoc, const fs::path& currentDir)
	{
		std::vector<fs::path> include_paths;
		collectIncludeElements(mainDoc, currentDir, include_paths);
		bool did_combine = false;

		for (const auto& include_path : include_paths)
		{
			XmlDocument included_doc;
			if (!included_doc.loadFile(include_path.string()))
			{
				throw XmlException("Failed to load included XML file: " + include_path.string());
			}

			mergeXmlDocuments(mainDoc, included_doc);
			did_combine = true;
		}

		// Remove all include elements from the main document
		removeIncludeElements(mainDoc);

		return did_combine;
	}

	/**
	 * @brief Validates the combined XML document using DTD and XSD schema data.
	 *
	 * @param didCombine True if any included documents were merged into the main document, false otherwise.
	 * @param mainDoc The combined XmlDocument to validate.
	 */
	void validateXml(const bool didCombine, const XmlDocument& mainDoc)
	{
		LOG(Level::DEBUG, "Validating the{}XML file...", didCombine ? " combined " : " ");
		// Validate the combined document using in-memory schema data - DTD validation is less strict than XSD
		if (!mainDoc.validateWithDtd(fers_xml_dtd))
		{
			LOG(Level::FATAL, "{} XML file failed DTD validation!", didCombine ? "Combined" : "Main");
			throw XmlException("XML file failed DTD validation!");
		}
		LOG(Level::DEBUG, "{} XML file passed DTD validation.", didCombine ? "Combined" : "Main");

		// Validate the combined document using in-memory schema data - XSD validation is stricter than DTD
		if (!mainDoc.validateWithXsd(fers_xml_xsd))
		{
			LOG(Level::FATAL, "{} XML file failed XSD validation!", didCombine ? "Combined" : "Main");
			throw XmlException("XML file failed XSD validation!");
		}
		LOG(Level::DEBUG, "{} XML file passed XSD validation.", didCombine ? "Combined" : "Main");
	}

	void processParsedDocument(const XmlDocument& doc, World* world, const fs::path& baseDir,
	                           std::mt19937& masterSeeder)
	{
		simulation_type = SimType::UNSET;

		const XmlElement root = doc.getRootElement();
		if (root.name() != "simulation")
		{
			throw XmlException("Root element is not <simulation>!");
		}

		try
		{
			params::params.simulation_name = XmlElement::getSafeAttribute(root, "name");
			if (!params::params.simulation_name.empty())
			{
				LOG(Level::INFO, "Simulation name set to: {}", params::params.simulation_name);
			}
		}
		catch (const XmlException&)
		{
			LOG(Level::WARNING, "No 'name' attribute found in <simulation> tag. KML name will default.");
		}

		parseParameters(root.childElement("parameters", 0));
		auto pulse_parser = [&](const XmlElement& p, World* w) { parsePulse(p, w, baseDir); };
		parseElements(root, "pulse", world, pulse_parser);
		parseElements(root, "timing", world, parseTiming);
		parseElements(root, "antenna", world, parseAntenna);

		auto platform_parser = [&](const XmlElement& p, World* w) { parsePlatform(p, w, masterSeeder); };
		parseElements(root, "platform", world, platform_parser);
	}
}

namespace serial
{
	void parseSimulation(const std::string& filename, World* world, const bool validate,
	                     std::mt19937& masterSeeder)
	{
		world->clear();
		params::params.reset();
		XmlDocument main_doc;
		if (!main_doc.loadFile(filename))
		{
			throw XmlException("Failed to load main XML file: " + filename);
		}

		const fs::path main_dir = fs::path(filename).parent_path();
		const bool did_combine = addIncludeFilesToMainDocument(main_doc, main_dir);

		if (validate) { validateXml(did_combine, main_doc); }
		else { LOG(Level::DEBUG, "Skipping XML validation."); }

		processParsedDocument(main_doc, world, main_dir, masterSeeder);
	}

	void parseSimulationFromString(const std::string& xml_content, World* world, const bool validate,
	                               std::mt19937& masterSeeder)
	{
		world->clear();
		params::params.reset();
		XmlDocument doc;
		if (!doc.loadString(xml_content))
		{
			throw XmlException("Failed to parse XML from memory string.");
		}

		if (validate)
		{
			// Note: <include> tags are not processed when loading from a string.
			validateXml(false, doc);
		}
		else { LOG(Level::DEBUG, "Skipping XML validation."); }

		// When loading from a string, there's no base directory for relative asset paths.
		// The UI/caller is responsible for ensuring any paths in the XML are absolute or resolvable.
		const fs::path base_dir = ".";

		processParsedDocument(doc, world, base_dir, masterSeeder);
	}
}
