//
// Created by David Young on 10/3/24.
//

#include "xml_parser.h"

#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "config.h"
#include "fers_xml_dtd.h"
#include "fers_xml_xsd.h"
#include "libxml_wrapper.h"
#include "antenna/antenna_factory.h"
#include "core/logging.h"
#include "core/parameters.h"
#include "core/world.h"
#include "math/coord.h"
#include "math/geometry_ops.h"
#include "math/multipath_surface.h"
#include "math/path.h"
#include "math/rotation_path.h"
#include "radar/platform.h"
#include "radar/radar_system.h"
#include "radar/target.h"
#include "serial/pulse_factory.h"
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
using signal::RadarSignal;
using timing::Timing;
using timing::PrototypeTiming;
using radar::Transmitter;
using radar::Receiver;

// Function to parse elements with child iteration (e.g., pulses, timings, antennas)
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

// Helper function to extract a RealType value from an element
auto get_child_real_type = [](const XmlElement& element, const std::string& elementName) -> RealType
{
	const std::string text = element.childElement(elementName, 0).getText();
	if (text.empty()) { throw XmlException("Element " + elementName + " is empty!"); }
	return std::stod(text);
};

// Helper function to extract a boolean value from an element
auto get_child_bool = [](const XmlElement& element, const std::string& elementName, const bool defaultVal) -> bool
{
	try { return XmlElement::getSafeAttribute(element, elementName) == "true"; }
	catch (const XmlException&)
	{
		LOG(Level::DEBUG, "Failed to get boolean value for element {}. Defaulting to {}.", elementName, defaultVal);
		return defaultVal;
	}
};

namespace
{
	// Function to parse the <parameters> element
	void parseParameters(const XmlElement& parameters)
	{
		// Set the start and end times
		params::setTime(get_child_real_type(parameters, "starttime"), get_child_real_type(parameters, "endtime"));

		auto set_param_with_exception_handling = [](const XmlElement& element, const std::string& paramName,
		                                            const RealType defaultValue,
		                                            const std::function<void(RealType)>& setter)
		{
			try
			{
				if (paramName == "randomseed" || paramName == "adc_bits" || paramName == "oversample")
				{
					setter(static_cast<unsigned>(std::floor(get_child_real_type(element, paramName))));
				}
				else { setter(get_child_real_type(element, paramName)); }
			}
			catch (const XmlException&)
			{
				LOG(Level::DEBUG, "Failed to set parameter {}. Using default value. {}", paramName, defaultValue);
			}
		};

		set_param_with_exception_handling(parameters, "c", params::c(), params::setC);
		set_param_with_exception_handling(parameters, "rate", params::rate(), params::setRate);
		set_param_with_exception_handling(parameters, "interprate", params::cwSampleRate(), params::setCwSampleRate);

		set_param_with_exception_handling(parameters, "randomseed", params::randomSeed(), params::setRandomSeed);
		set_param_with_exception_handling(parameters, "adc_bits", params::adcBits(), params::setAdcBits);
		set_param_with_exception_handling(parameters, "oversample", params::oversampleRatio(),
		                                  params::setOversampleRatio);

		// Set exporters
		if (const XmlElement export_element = parameters.childElement("export", 0); export_element.isValid())
		{
			params::setExporters(
				get_child_bool(export_element, "xml", params::exportXml()),
				get_child_bool(export_element, "csv", params::exportCsv()),
				get_child_bool(export_element, "binary", params::exportBinary())
			);
		}
	}

	// Function to parse individual <pulse> elements
	void parsePulse(const XmlElement& pulse, World* world)
	{
		const std::string name = XmlElement::getSafeAttribute(pulse, "name");
		const std::string type = XmlElement::getSafeAttribute(pulse, "type");
		const std::string filename = XmlElement::getSafeAttribute(pulse, "filename");

		if (const XmlElement power_element = pulse.childElement("power", 0); !power_element.isValid())
		{
			std::cerr << "  <power> element is missing in <pulse>!" << std::endl;
			return;
		}

		if (const XmlElement carrier_element = pulse.childElement("carrier", 0); !carrier_element.isValid())
		{
			std::cerr << "  <carrier> element is missing in <pulse>!" << std::endl;
			return;
		}

		if (type == "file")
		{
			auto wave = serial::loadPulseFromFile(name, filename,
			                                      get_child_real_type(pulse, "power"),
			                                      get_child_real_type(pulse, "carrier")
			);
			world->add(std::move(wave));
		}
		else { std::cerr << "  Unsupported pulse type: " << type << std::endl; }
	}

	// Function to parse individual <timing> elements
	void parseTiming(const XmlElement& timing, World* world)
	{
		// Extract required attribute: name
		const std::string name = XmlElement::getSafeAttribute(timing, "name");
		auto timing_obj = std::make_unique<PrototypeTiming>(name);

		// Extract noise entries (optional)
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
		catch (XmlException&) { LOG(Level::DEBUG, "Clock section '{}' does not specify frequency offset.", name); }

		try { timing_obj->setRandomFreqOffset(get_child_real_type(timing, "random_freq_offset")); }
		catch (XmlException&)
		{
			LOG(Level::DEBUG, "Clock section '{}' does not specify random frequency offset.", name);
		}

		try { timing_obj->setPhaseOffset(get_child_real_type(timing, "phase_offset")); }
		catch (XmlException&) { LOG(Level::DEBUG, "Clock section '{}' does not specify phase offset.", name); }

		try { timing_obj->setRandomPhaseOffset(get_child_real_type(timing, "random_phase_offset")); }
		catch (XmlException&) { LOG(Level::DEBUG, "Clock section '{}' does not specify random phase offset.", name); }

		try { timing_obj->setFrequency(get_child_real_type(timing, "frequency")); }
		catch (XmlException&)
		{
			timing_obj->setFrequency(params::rate());
			LOG(Level::DEBUG, "Clock section '{}' does not specify frequency. Assuming {}.", name, params::rate());
		}

		// Extract optional attribute: synconpulse (default is "true")
		if (get_child_bool(timing, "synconpulse", true)) { timing_obj->setSyncOnPulse(); }

		world->add(std::move(timing_obj));
	}

	// Function to parse individual <antenna> elements
	void parseAntenna(const XmlElement& antenna, World* world)
	{
		// Extract required attributes: name, pattern
		std::string name = XmlElement::getSafeAttribute(antenna, "name");
		const std::string pattern = XmlElement::getSafeAttribute(antenna, "pattern");

		std::unique_ptr<Antenna> ant;

		// Create the appropriate antenna object based on the pattern
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
			ant = std::make_unique<antenna::FileAntenna>(name, XmlElement::getSafeAttribute(antenna, "filename"));
		}
		else if (pattern == "python")
		{
			ant = std::make_unique<antenna::PythonAntenna>(name,
			                                               XmlElement::getSafeAttribute(antenna, "module"),
			                                               XmlElement::getSafeAttribute(antenna, "function")
			);
		}
		else
		{
			std::cerr << "  Unsupported antenna pattern: " << pattern << std::endl;
			return;
		}

		// Set the <efficiency> element (optional)
		try { ant->setEfficiencyFactor(get_child_real_type(antenna, "efficiency")); }
		catch (XmlException&) { LOG(Level::DEBUG, "Antenna '{}' does not specify efficiency, assuming unity.", name); }

		world->add(std::move(ant));
	}

	// Function to parse individual <motionpath> element
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
			else if (interp == "python")
			{
				path->setInterp(Path::InterpType::INTERP_PYTHON);
				const XmlElement python_path = motionPath.childElement("pythonpath", 0);
				path->setPythonPath(
					XmlElement::getSafeAttribute(python_path, "module"),
					XmlElement::getSafeAttribute(python_path, "function")
				);
			}
			else
			{
				LOG(Level::ERROR, "Unsupported interpolation type: {} for platform {}. Defaulting to static", interp,
				    platform->getName());
				path->setInterp(Path::InterpType::INTERP_STATIC);
			}
		}
		catch (XmlException&)
		{
			LOG(Level::ERROR, "Failed to set interpolation type for platform {}. Defaulting to static",
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

	// Function to parse <rotationpath> element
	void parseRotationPath(const XmlElement& rotation, const Platform* platform)
	{
		RotationPath* path = platform->getRotationPath();

		try
		{
			if (std::string interp = XmlElement::getSafeAttribute(rotation, "interpolation"); interp == "linear")
			{
				path->setInterp(RotationPath::InterpType::INTERP_LINEAR);
			}
			else if (interp == "cubic") { path->setInterp(RotationPath::InterpType::INTERP_CUBIC); }
			else if (interp == "static") { path->setInterp(RotationPath::InterpType::INTERP_STATIC); }
			else
			{
				LOG(Level::ERROR, "Unsupported interpolation type: {} for platform {}. Defaulting to static", interp,
				    platform->getName());
				path->setInterp(RotationPath::InterpType::INTERP_STATIC);
			}
		}
		catch (XmlException&)
		{
			LOG(Level::ERROR, "Failed to set interpolation type for platform {}. Defaulting to static",
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
				path->addCoord({
					get_child_real_type(waypoint, "elevation"),
					get_child_real_type(waypoint, "azimuth"),
					get_child_real_type(waypoint, "time")
				});
			}
			catch (const XmlException& e)
			{
				LOG(Level::ERROR, "Failed to add waypoint to rotation path. Discarding waypoint. {}", e.what());
			}

			waypoint_index++;
		}

		path->finalize();
	}

	// Function to parse <fixedrotation> element
	void parseFixedRotation(const XmlElement& rotation, const Platform* platform)
	{
		RotationPath* path = platform->getRotationPath();
		try
		{
			math::RotationCoord start, rate;
			start.azimuth = get_child_real_type(rotation, "startazimuth");
			start.elevation = get_child_real_type(rotation, "startelevation");
			rate.azimuth = get_child_real_type(rotation, "azimuthrate");
			rate.elevation = get_child_real_type(rotation, "elevationrate");
			path->setConstantRate(start, rate);
			LOG(Level::DEBUG, "Added fixed rotation to platform {}", platform->getName());
		}
		catch (XmlException& e)
		{
			LOG(Level::ERROR, "Failed to set fixed rotation for platform {}. {}", platform->getName(), e.what());
		}
	}

	// Function to parse individual <transmitter> element
	Transmitter* parseTransmitter(const XmlElement& transmitter, Platform* platform, World* world)
	{
		// Required attributes
		const std::string name = XmlElement::getSafeAttribute(transmitter, "name");
		bool pulsed = XmlElement::getSafeAttribute(transmitter, "type") == "pulsed";

		auto transmitter_obj = std::make_unique<Transmitter>(platform, name, pulsed);

		const std::string pulse_name = XmlElement::getSafeAttribute(transmitter, "pulse");
		RadarSignal* pulse = world->findSignal(pulse_name);
		transmitter_obj->setWave(pulse);

		transmitter_obj->setPrf(get_child_real_type(transmitter, "prf"));

		const std::string antenna_name = XmlElement::getSafeAttribute(transmitter, "antenna");
		const Antenna* ant = world->findAntenna(antenna_name);
		transmitter_obj->setAntenna(ant);

		const std::string timing_name = XmlElement::getSafeAttribute(transmitter, "timing");
		const auto timing = std::make_shared<Timing>(name);
		const PrototypeTiming* proto = world->findTiming(timing_name);
		timing->initializeModel(proto);
		transmitter_obj->setTiming(timing);

		world->add(std::move(transmitter_obj));

		return world->getTransmitters().back().get();
	}

	// Function to parse individual <receiver> element
	Receiver* parseReceiver(const XmlElement& receiver, Platform* platform, World* world)
	{
		const std::string name = XmlElement::getSafeAttribute(receiver, "name");

		auto receiver_obj = std::make_unique<Receiver>(platform, name);

		const std::string ant_name = XmlElement::getSafeAttribute(receiver, "antenna");

		const Antenna* antenna = world->findAntenna(ant_name);
		receiver_obj->setAntenna(antenna);

		try { receiver_obj->setNoiseTemperature(get_child_real_type(receiver, "noise_temp")); }
		catch (XmlException&)
		{
			LOG(Level::INFO, "Receiver '{}' does not specify noise temperature",
			    receiver_obj->getName().c_str());
		}

		receiver_obj->setWindowProperties(
			get_child_real_type(receiver, "window_length"),
			get_child_real_type(receiver, "prf"),
			get_child_real_type(receiver, "window_skip")
		);

		const std::string timing_name = XmlElement::getSafeAttribute(receiver, "timing");
		const auto timing = std::make_shared<Timing>(name);

		const PrototypeTiming* proto = world->findTiming(timing_name);
		timing->initializeModel(proto);
		receiver_obj->setTiming(timing);

		if (get_child_bool(receiver, "nodirect", false))
		{
			receiver_obj->setFlag(Receiver::RecvFlag::FLAG_NODIRECT);
			LOG(Level::DEBUG, "Ignoring direct signals for receiver '{}'",
			    receiver_obj->getName().c_str());
		}

		if (get_child_bool(receiver, "nopropagationloss", false))
		{
			receiver_obj->setFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS);
			LOG(Level::DEBUG, "Ignoring propagation losses for receiver '{}'",
			    receiver_obj->getName().c_str());
		}

		world->add(std::move(receiver_obj));
		return world->getReceivers().back().get();
	}

	// Function to parse individual <monostatic> radar element
	void parseMonostatic(const XmlElement& monostatic, Platform* platform, World* world)
	{
		// Create the transmitter and receiver objects
		Transmitter* trans = parseTransmitter(monostatic, platform, world);
		Receiver* recv = parseReceiver(monostatic, platform, world);
		// Attach the transmitter and receiver objects
		trans->setAttached(recv);
		recv->setAttached(trans);
	}

	// Function to parse individual <target> element
	void parseTarget(const XmlElement& target, Platform* platform, World* world)
	{
		// Required attribute of <target>
		const std::string name = XmlElement::getSafeAttribute(target, "name");

		// Required <rcs> element
		const XmlElement rcs_element = target.childElement("rcs", 0);
		if (!rcs_element.isValid()) { throw XmlException("<rcs> element is required in <target>!"); }

		const std::string rcs_type = XmlElement::getSafeAttribute(rcs_element, "type");

		std::unique_ptr<Target> target_obj;

		if (rcs_type == "isotropic")
		{
			target_obj = createIsoTarget(platform, name, get_child_real_type(rcs_element, "value"));
		}
		else if (rcs_type == "file")
		{
			target_obj = createFileTarget(platform, name, XmlElement::getSafeAttribute(rcs_element, "filename"));
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
					get_child_real_type(model, "k")
				));
			}
			else { throw XmlException("Unsupported model type: " + model_type); }
		}

		LOG(Level::DEBUG, "Added target {} with RCS type {} to platform {}", name, rcs_type, platform->getName());

		world->add(std::move(target_obj));
	}

	// Function to parse the <platform> element
	void parsePlatform(const XmlElement& platform, World* world)
	{
		std::string name = XmlElement::getSafeAttribute(platform, "name");
		auto plat = std::make_unique<Platform>(name);

		// Parse optional <monostatic>, <transmitter>, <receiver>, and <target>
		unsigned element_index = 0;
		while (true)
		{
			XmlElement monostatic = platform.childElement("monostatic", element_index);
			XmlElement transmitter = platform.childElement("transmitter", element_index);
			XmlElement receiver = platform.childElement("receiver", element_index);
			XmlElement target = platform.childElement("target", element_index);

			if (target.isValid()) { parseTarget(target, plat.get(), world); }
			if (transmitter.isValid()) { parseTransmitter(transmitter, plat.get(), world); }
			if (receiver.isValid()) { parseReceiver(receiver, plat.get(), world); }
			if (monostatic.isValid()) { parseMonostatic(monostatic, plat.get(), world); }

			if (!monostatic.isValid() && !transmitter.isValid() && !receiver.isValid() && !target.isValid()) { break; }

			element_index++;
		}

		// Parse <motionpath> (required)
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
		else if (fixed_rot.isValid()) { parseFixedRotation(fixed_rot, plat.get()); }

		world->add(std::move(plat));
	}

	// Function to parse individual <multipath> element
	void parseMultipathSurface(const XmlElement& multipathSurface, World* world)
	{
		auto mps = std::make_unique<math::MultipathSurface>(
			get_child_real_type(multipathSurface, "nx"),
			get_child_real_type(multipathSurface, "ny"),
			get_child_real_type(multipathSurface, "nz"),
			get_child_real_type(multipathSurface, "d"),
			get_child_real_type(multipathSurface, "factor")
		);
		world->addMultipathSurface(std::move(mps));
	}

	// Function to collect all includes elements from the main document and included documents
	void collectIncludeElements(const XmlDocument& doc, const fs::path& currentDir, std::vector<fs::path>& includePaths) // NOLINT(misc-no-recursion)
	{
		unsigned index = 0;
		while (true)
		{
			XmlElement include_element = doc.getRootElement().childElement("include", index++);
			if (!include_element.isValid()) { break; }

			// Get the filename from the <include> element
			std::string include_filename = include_element.getText();
			if (include_filename.empty())
			{
				std::cerr << "<include> element is missing the filename!" << std::endl;
				continue;
			}

			// Construct the full path to the included file
			fs::path include_path = currentDir / include_filename;
			includePaths.push_back(include_path);

			// Load the included XML file
			XmlDocument included_doc;
			if (!included_doc.loadFile(include_path.string()))
			{
				std::cerr << "Failed to load included XML file: " << include_path << std::endl;
				continue;
			}

			// Recursively collect include elements from the included document
			collectIncludeElements(included_doc, include_path.parent_path(), includePaths);
		}
	}

	void addIncludeFilesToMainDocument(const XmlDocument& mainDoc, const fs::path& currentDir)
	{
		std::vector<fs::path> include_paths;
		collectIncludeElements(mainDoc, currentDir, include_paths);

		for (const auto& include_path : include_paths)
		{
			// Load the included XML file
			XmlDocument included_doc;
			if (!included_doc.loadFile(include_path.string()))
			{
				std::cerr << "Failed to load included XML file: " << include_path << std::endl;
				continue;
			}

			// Merge the included document into the main document
			mergeXmlDocuments(mainDoc, included_doc);
		}

		// Remove all include elements from the main document
		removeIncludeElements(mainDoc);
	}
}

namespace serial
{
	// Function to parse the entire <simulation> element
	void parseSimulation(const std::string& filename, World* world)
	{
		// Load the main simulation XML file
		XmlDocument main_doc;
		if (!main_doc.loadFile(filename))
		{
			std::cerr << "Failed to load main XML file: " << filename << std::endl;
			return;
		}

		// Get the directory of the main XML file to resolve include paths
		const fs::path main_dir = fs::path(filename).parent_path();

		// Add all included files into the main document
		const XmlElement root = main_doc.getRootElement();
		if (root.name() != "simulation")
		{
			LOG(Level::FATAL, "Root element is not <simulation>!");
			throw XmlException("Root element is not <simulation>!");
		}

		// Process <include> elements and merge their contents into the main document
		addIncludeFilesToMainDocument(main_doc, main_dir);

		// Validate the combined document using in-memory schema data - DTD validation is less strict than XSD
		if (!main_doc.validateWithDtd(fers_xml_dtd))
		{
			LOG(Level::FATAL, "Combined XML file failed DTD validation!");
			throw XmlException("Combined XML file failed DTD validation!");
		}
		LOG(Level::DEBUG, "Combined XML file passed DTD validation.");

		// Validate the combined document using in-memory schema data - XSD validation is stricter than DTD
		if (!main_doc.validateWithXsd(fers_xml_xsd))
		{
			LOG(Level::FATAL, "Combined XML file failed XSD validation!");
			throw XmlException("Combined XML file failed XSD validation!");
		}
		LOG(Level::DEBUG, "Combined XML file passed XSD validation.");

		// Proceed with parsing the main document's contents
		parseParameters(root.childElement("parameters", 0));
		parseElements(root, "pulse", world, parsePulse);
		parseElements(root, "timing", world, parseTiming);
		parseElements(root, "antenna", world, parseAntenna);
		parseElements(root, "platform", world, parsePlatform);
		parseElements(root, "multipath", world, parseMultipathSurface);

		world->processMultipath();
	}
}
