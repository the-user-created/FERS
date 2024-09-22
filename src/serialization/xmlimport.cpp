// xmlimport.cpp
// Import a simulator world and simulation parameters from an XML file
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started 26 April 2006

#define TIXML_USE_STL

#include "xmlimport.h"

#include <cmath>                             // for floor, fabs
#include <functional>                        // for function
#include <memory>                            // for unique_ptr, make_unique
#include <sstream>                           // for basic_istringstream, ope...
#include <stdexcept>                         // for runtime_error
#include <tinyxml.h>                         // for TiXmlHandle, TiXmlElement
#include <utility>                           // for move
#include <vector>                            // for vector

#include "config.h"                          // for RealType
#include "antenna/antenna_factory.h"         // for Antenna, FileAntenna
#include "core/logging.h"                    // for log, LOG, Level
#include "core/parameters.h"                 // for rate, adcBits, c, cwSamp...
#include "core/world.h"                      // for World
#include "math_utils/coord.h"                // for RotationCoord, Coord
#include "math_utils/geometry_ops.h"         // for Vec3
#include "math_utils/multipath_surface.h"    // for MultipathSurface
#include "math_utils/path.h"                 // for Path
#include "math_utils/rotation_path.h"        // for RotationPath
#include "radar/platform.h"                  // for Platform
#include "radar/radar_system.h"              // for Transmitter, Receiver
#include "radar/target.h"                    // for RcsChiSquare, RcsConst
#include "serialization/pulse_factory.h"     // for loadPulseFromFile
#include "signal_processing/radar_signal.h"  // for RadarSignal
#include "timing/prototype_timing.h"         // for PrototypeTiming
#include "timing/timing.h"                   // for Timing

using radar::Platform;
using radar::Receiver;
using radar::Transmitter;
using radar::Target;
using timing::Timing;
using timing::PrototypeTiming;
using signal::RadarSignal;
using core::World;
using antenna::Antenna;
using logging::Level;

// =====================================================================================================================
//
// XML PARSING UTILITY FUNCTIONS
//
// =====================================================================================================================

/// Exception for reporting an XML parsing error
class XmlImportException final : public std::runtime_error
{
public:
	explicit XmlImportException(const std::string& error):
		std::runtime_error("Error while parsing XML file: " + error) {}
};

/// Function which takes a TiXmlHandle and returns the text contained in it's children.
//For processing XML like this:
//<tree>
//<leaf1>Green</leaf1>
//<leaf2>Blue</leaf2>
//</tree>
//Pass a handle to tree, and the string "leaf1" to get "Green"
const char* getChildText(const TiXmlHandle& parent, const char* childname)
{
	const TiXmlElement* element = parent.FirstChildElement(childname).Element();
	return element ? element->GetText() : nullptr;
}

/// Gets child text as a rsFloat. See GetChildText for usage description
RealType getChildRsFloat(const TiXmlHandle& parent, const char* childname)
{
	const char* data = getChildText(parent, childname);
	if (!data)
	{
		throw XmlImportException("No data in child element " + std::string(childname) + " during GetChildRsFloat.");
	}
	RealType result;
	std::istringstream(data) >> result;
	return result;
}

/// Gets the text content of a node as an rsFloat.
//For XML like this:
//<rcs>10</rcs>
RealType getNodeFloat(const TiXmlHandle& node)
{
	const char* data = node.Element() ? node.Element()->GetText() : nullptr;
	if (!data) { throw XmlImportException("Node does not contain text during GetNodeFloat"); }
	RealType result;
	std::istringstream(data) >> result;
	return result;
}

/// Return the string associated with an attribute or throw an exception on failure
std::string getAttributeString(const TiXmlHandle& handle, const std::string& name, const std::string& error,
                               const bool optional = false)
{
	if (const std::string* text = handle.Element()->Attribute(name)) { return *text; }
	if (!optional) { throw XmlImportException(error); }
	return {};
}

/// Return the bool associated with an attribute
bool getAttributeBool(const TiXmlHandle& handle, const std::string& name, const std::string& error, const bool def,
                      const bool optional = true)
{
	const std::string str = getAttributeString(handle, name, error, optional);
	return str.empty() ? def : str == "true" || str == "yes";
}

namespace
{
	/// Process a target XML entry
	void processTarget(const TiXmlHandle& targXml, Platform* platform, World* world)
	{
		const std::string name = getAttributeString(targXml, "name", "Target does not specify a name");
		const TiXmlHandle rcs_xml = targXml.ChildElement("rcs", 0);
		if (!rcs_xml.Element()) { throw XmlImportException("Target " + name + " does not specify RCS."); }

		const std::string rcs_type = getAttributeString(rcs_xml, "type",
		                                                "RCS attached to target '" + name + "' does not specify type.");
		std::unique_ptr<Target> target;

		if (rcs_type == "isotropic")
		{
			const TiXmlHandle rcs_value_xml = rcs_xml.ChildElement("value", 0);
			if (!rcs_value_xml.Element())
			{
				throw XmlImportException("Target " + name + " does not specify value of isotropic RCS.");
			}
			target = createIsoTarget(platform, name, getNodeFloat(rcs_value_xml));
		}
		else if (rcs_type == "file")
		{
			const std::string filename = getAttributeString(rcs_xml, "filename",
			                                                "RCS attached to target '" + name +
			                                                "' does not specify filename.");
			target = createFileTarget(platform, name, filename);
		}
		else { throw XmlImportException("RCS type " + rcs_type + " not currently supported."); }

		if (const TiXmlHandle model_xml = targXml.ChildElement("model", 0); model_xml.Element())
		{
			const std::string model_type = getAttributeString(model_xml, "type",
			                                                  "Model attached to target '" + name +
			                                                  "' does not specify type.");
			if (model_type == "constant") { target->setFluctuationModel(std::make_unique<radar::RcsConst>()); }
			else if (model_type == "chisquare" || model_type == "gamma")
			{
				target->setFluctuationModel(std::make_unique<radar::RcsChiSquare>(getChildRsFloat(model_xml, "k")));
			}
			else { throw XmlImportException("Target fluctuation model type '" + model_type + "' not recognised."); }
		}

		world->add(std::move(target));
	}

	/// Process a receiver XML entry
	Receiver* processReceiver(const TiXmlHandle& recvXml, Platform* platform, World* world)
	{
		const std::string name = getAttributeString(recvXml, "name", "Receiver does not specify a name");
		auto receiver = std::make_unique<Receiver>(platform, name);

		const std::string ant_name = getAttributeString(recvXml, "antenna",
		                                                "Receiver '" + name + "' does not specify an antenna");

		const Antenna* antenna = world->findAntenna(ant_name);
		if (!antenna)
		{
			throw XmlImportException(
				"Antenna with name '" + ant_name + "' does not exist when processing Receiver " + name);
		}
		receiver->setAntenna(antenna);

		try { receiver->setNoiseTemperature(getChildRsFloat(recvXml, "noise_temp")); }
		catch (XmlImportException&)
		{
			LOG(Level::INFO, "Receiver '{}' does not specify noise temperature",
			    receiver->getName().c_str());
		}

		receiver->setWindowProperties(
			getChildRsFloat(recvXml, "window_length"),
			getChildRsFloat(recvXml, "prf"),
			getChildRsFloat(recvXml, "window_skip")
		);

		const std::string timing_name = getAttributeString(recvXml, "timing",
		                                                   "Receiver '" + name + "' does not specify a timing source");
		const auto timing = std::make_shared<Timing>(name);

		const PrototypeTiming* proto = world->findTiming(timing_name);
		if (!proto)
		{
			throw XmlImportException(
				"Timing source '" + timing_name + "' does not exist when processing receiver '" + name + "'");
		}
		timing->initializeModel(proto);
		receiver->setTiming(timing);

		if (getAttributeBool(recvXml, "nodirect", "", false))
		{
			receiver->setFlag(Receiver::RecvFlag::FLAG_NODIRECT);
			LOG(Level::DEBUG, "Ignoring direct signals for receiver '{}'",
			    receiver->getName().c_str());
		}

		if (getAttributeBool(recvXml, "nopropagationloss", "", false))
		{
			receiver->setFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS);
			LOG(Level::DEBUG, "Ignoring propagation losses for receiver '{}'",
			    receiver->getName().c_str());
		}

		world->add(std::move(receiver));
		return world->getReceivers().back().get();
	}

	/// Create a PulseTransmitter object and process XML entry
	std::unique_ptr<Transmitter> processPulseTransmitter(const TiXmlHandle& transXml, const std::string& name, Platform* platform, World* world)
	{
		auto transmitter = std::make_unique<Transmitter>(platform, name, true);
		const std::string pulse_name = getAttributeString(transXml, "pulse",
		                                                  "Transmitter '" + name + "' does not specify a pulse");
		RadarSignal* wave = world->findSignal(pulse_name);
		if (!wave) { throw XmlImportException("Pulse with name '" + pulse_name + "' does not exist"); }
		transmitter->setWave(wave);
		transmitter->setPrf(getChildRsFloat(transXml, "prf"));
		return transmitter;
	}

	/// Create a PulseTransmitter object and process XML entry
	std::unique_ptr<Transmitter> processCwTransmitter(const TiXmlHandle& transXml, const std::string& name, Platform* platform, World* world)
	{
		auto transmitter = std::make_unique<Transmitter>(platform, name, true);
		const std::string pulse_name = getAttributeString(transXml, "pulse",
		                                                  "Transmitter '" + name + "' does not specify a pulse");
		RadarSignal* wave = world->findSignal(pulse_name);
		if (!wave) { throw XmlImportException("Pulse with name '" + pulse_name + "' does not exist"); }
		transmitter->setWave(wave);
		// TODO: without setting the PRF, a continuous wave transmitter will get 0 responses
		transmitter->setPrf(getChildRsFloat(transXml, "prf"));
		return transmitter;
	}

	/// Process a transmitter XML entry
	Transmitter* processTransmitter(const TiXmlHandle& transXml, Platform* platform, World* world)
	{
		const std::string name = getAttributeString(transXml, "name", "Transmitter does not specify a name");
		const std::string type = getAttributeString(transXml, "type",
		                                            "Transmitter '" + name + "' does not specify type");

		auto transmitter = type == "pulsed"
			                   ? processPulseTransmitter(transXml, name, platform, world)
			                   : type == "continuous"
			                   ? processCwTransmitter(transXml, name, platform, world)
			                   : throw XmlImportException(
				                   "Invalid transmitter type specified in transmitter " + name);

		const std::string ant_name = getAttributeString(transXml, "antenna",
		                                                "Transmitter '" + name + "' does not specify an antenna");
		const Antenna* antenna = world->findAntenna(ant_name);
		if (!antenna)
		{
			throw XmlImportException(
				"Antenna with name '" + ant_name + "' does not exist when processing Transmitter " + name);
		}
		transmitter->setAntenna(antenna);

		const std::string timing_name = getAttributeString(transXml, "timing",
		                                                   "Transmitter '" + name +
		                                                   "' does not specify a timing source");
		const auto timing = std::make_shared<Timing>(name);
		const PrototypeTiming* proto = world->findTiming(timing_name);
		if (!proto)
		{
			throw XmlImportException(
				"Timing source '" + timing_name + "' does not exist when processing receiver " + name);
		}
		timing->initializeModel(proto);
		transmitter->setTiming(timing);

		world->add(std::move(transmitter));

		return world->getTransmitters().back().get();
	}

	/// Process a monostatic (Receiver and Transmitter sharing an antenna)
	void processMonostatic(const TiXmlHandle& transXml, Platform* platform, World* world)
	{
		Transmitter* trans = processTransmitter(transXml, platform, world);
		Receiver* recv = processReceiver(transXml, platform, world);
		trans->setAttached(recv);
		recv->setAttached(trans);
	}

	/// Process a motion path waypoint
	void processWaypoint(const TiXmlHandle& handXml, math::Path* path)
	{
		try
		{
			const RealType x = getChildRsFloat(handXml, "x");
			const RealType y = getChildRsFloat(handXml, "y");
			const RealType z = getChildRsFloat(handXml, "altitude");
			const RealType t = getChildRsFloat(handXml, "time");
			math::Coord coord;
			coord.t = t;
			coord.pos = math::Vec3(x, y, z);
			path->addCoord(coord);
		}
		catch (XmlImportException& e)
		{
			LOG(Level::ERROR, "Parse Error While Importing Waypoint. Discarding Waypoint.{}", e.what());
		}
	}

	/// Process the path's python attributes
	void processPythonPath(const TiXmlHandle& pathXml, math::Path* path)
	{
		try
		{
			const TiXmlHandle tmp = pathXml.FirstChildElement("pythonpath");
			path->setPythonPath(
				getAttributeString(tmp, "module", "Attribute module missing"),
				getAttributeString(tmp, "function", "Attribute function missing")
			);
		}
		catch (const XmlImportException& e) { LOG(Level::DEBUG, "{}", e.what()); }
	}

	/// Process a MotionPath XML entry
	void processMotionPath(const TiXmlHandle& mpXml, const Platform* platform)
	{
		math::Path* path = platform->getMotionPath();

		try
		{
			if (const std::string rottype = getAttributeString(mpXml, "interpolation", ""); rottype == "linear")
			{
				path->setInterp(math::Path::InterpType::INTERP_LINEAR);
			}
			else if (rottype == "cubic") { path->setInterp(math::Path::InterpType::INTERP_CUBIC); }
			else if (rottype == "static") { path->setInterp(math::Path::InterpType::INTERP_STATIC); }
			else if (rottype == "python")
			{
				path->setInterp(math::Path::InterpType::INTERP_PYTHON);
				processPythonPath(mpXml, path);
			}
			else
			{
				LOG(Level::ERROR,
				    "Unsupported motion path interpolation type for platform '{}'. Defaulting to static.",
				    platform->getName().c_str());
				path->setInterp(math::Path::InterpType::INTERP_STATIC);
			}
		}
		catch (XmlImportException&)
		{
			LOG(Level::ERROR,
			    "Motion path interpolation type not specified for platform '{}'. Defaulting to static.",
			    platform->getName().c_str());
			path->setInterp(math::Path::InterpType::INTERP_STATIC);
		}

		int i{};
		for (TiXmlHandle tmp = mpXml.ChildElement("positionwaypoint", 0); tmp.Element();
		     tmp = mpXml.ChildElement("positionwaypoint", ++i)) { processWaypoint(tmp, path); }

		path->finalize();
	}

	/// Process a rotation path waypoint
	void processRotationWaypoint(const TiXmlHandle& handXml, math::RotationPath* path)
	{
		try
		{
			path->addCoord({
				getChildRsFloat(handXml, "elevation"),
				getChildRsFloat(handXml, "azimuth"),
				getChildRsFloat(handXml, "time")
			});
		}
		catch (const XmlImportException& e)
		{
			LOG(Level::ERROR, "Parse Error While Importing Waypoint. Discarding Waypoint.{}", e.what());
		}
	}

	/// Process Waypoints for RotationPath
	void processRotationWaypoints(const TiXmlHandle& mpXml, math::RotationPath* path)
	{
		for (TiXmlHandle tmp = mpXml.ChildElement("rotationwaypoint", 0); tmp.Element(); tmp = tmp.Element()->
		     NextSiblingElement("rotationwaypoint")) { processRotationWaypoint(tmp, path); }
		path->finalize();
	}

	/// Process an entry for a fixed rotation
	void processRotationConstant(const TiXmlHandle& mpXml, const Platform* platform)
	{
		math::RotationPath* path = platform->getRotationPath();
		try
		{
			math::RotationCoord start, rate;
			start.azimuth = getChildRsFloat(mpXml, "startazimuth");
			start.elevation = getChildRsFloat(mpXml, "startelevation");
			rate.azimuth = getChildRsFloat(mpXml, "azimuthrate");
			rate.elevation = getChildRsFloat(mpXml, "elevationrate");
			path->setConstantRate(start, rate);
		}
		catch (XmlImportException& e)
		{
			LOG(Level::ERROR, "Parse Error While Importing Constant Rotation.{}", e.what());
		}
	}

	/// Process a RotationPath XML entry
	void processRotationPath(const TiXmlHandle& mpXml, const Platform* platform)
	{
		math::RotationPath* path = platform->getRotationPath();

		try
		{
			if (const std::string rottype = getAttributeString(mpXml, "interpolation", ""); rottype == "linear")
			{
				path->setInterp(math::RotationPath::InterpType::INTERP_LINEAR);
			}
			else if (rottype == "cubic") { path->setInterp(math::RotationPath::InterpType::INTERP_CUBIC); }
			else if (rottype == "static") { path->setInterp(math::RotationPath::InterpType::INTERP_STATIC); }
			else
			{
				LOG(Level::ERROR,
				    "Unsupported rotation path interpolation type for platform '{}'. Defaulting to static.",
				    platform->getName().c_str());
				path->setInterp(math::RotationPath::InterpType::INTERP_STATIC);
			}
		}
		catch (XmlImportException&)
		{
			LOG(Level::ERROR,
			    "Rotation path interpolation type not specified for platform '{}'. Defaulting to static.",
			    platform->getName().c_str());
			path->setInterp(math::RotationPath::InterpType::INTERP_STATIC);
		}

		processRotationWaypoints(mpXml, path);
	}

	/// Process a platform, recursively processing all the elements that are attached to it
	void processPlatform(const TiXmlHandle& platXml, World* world)
	{
		const std::string name = getAttributeString(platXml, "name", "Platform must specify a name");
		auto platform = std::make_unique<Platform>(name);

		int i{};
		for (TiXmlHandle tmp = platXml.ChildElement("target", 0); tmp.Element();
		     tmp = platXml.ChildElement("target", ++i)) { processTarget(tmp, platform.get(), world); }

		i = 0;
		for (TiXmlHandle tmp = platXml.ChildElement("receiver", 0); tmp.Element();
		     tmp = platXml.ChildElement("receiver", ++i)) { processReceiver(tmp, platform.get(), world); }

		i = 0;
		for (TiXmlHandle tmp = platXml.ChildElement("transmitter", 0); tmp.Element();
		     tmp = platXml.ChildElement("transmitter", ++i)) { processTransmitter(tmp, platform.get(), world); }

		i = 0;
		for (TiXmlHandle tmp = platXml.ChildElement("monostatic", 0); tmp.Element();
		     tmp = platXml.ChildElement("monostatic", ++i)) { processMonostatic(tmp, platform.get(), world); }

		i = 0;
		for (TiXmlHandle tmp = platXml.ChildElement("motionpath", 0); tmp.Element();
		     tmp = platXml.ChildElement("motionpath", ++i)) { processMotionPath(tmp, platform.get()); }

		i = 0;
		for (TiXmlHandle tmp = platXml.ChildElement("rotationpath", 0); tmp.Element();
		     tmp = platXml.ChildElement("rotationpath", ++i)) { processRotationPath(tmp, platform.get()); }

		i = 0;
		for (TiXmlHandle tmp = platXml.ChildElement("fixedrotation", 0); tmp.Element();
		     tmp = platXml.ChildElement("fixedrotation", ++i)) { processRotationConstant(tmp, platform.get()); }

		world->add(std::move(platform));
	}

	/// Process a pulse entry of type rect
	void processAnyPulseFile(const TiXmlHandle& pulseXml, World* world, const std::string& name)
	{
		auto wave = serial::loadPulseFromFile(
			name,
			getAttributeString(pulseXml, "filename", "Pulse must specify a filename"),
			getChildRsFloat(pulseXml, "power"),
			getChildRsFloat(pulseXml, "carrier")
		);
		world->add(std::move(wave));
	}

	/// Process a pulse entry
	void processPulse(const TiXmlHandle& pulseXml, World* world)
	{
		const std::string pulse_name = getAttributeString(pulseXml, "name", "Pulses must specify a name");
		const std::string pulse_type = getAttributeString(pulseXml, "type", "Pulses must specify a type");
		LOG(Level::DEBUG, "Generating Pulse {} of type '{}'", pulse_name.c_str(),
		    pulse_type.c_str());

		if (pulse_type == "file") { processAnyPulseFile(pulseXml, world, pulse_name); }
		else { throw XmlImportException("Unrecognised type in pulse"); }
	}

	std::unique_ptr<Antenna> createAntenna(const std::string& antPattern, const TiXmlHandle& antXml,
	                                       const std::string& antName)
	{
		if (antPattern == "isotropic") { return std::make_unique<antenna::Isotropic>(antName); }
		if (antPattern == "file")
		{
			return std::make_unique<antenna::FileAntenna>(
				antName, getAttributeString(antXml, "filename", "File antenna must specify a file"));
		}
		if (antPattern == "xml")
		{
			return std::make_unique<antenna::XmlAntenna>(
				antName, getAttributeString(antXml, "filename", "Xml antenna must specify a file"));
		}
		if (antPattern == "python")
		{
			return std::make_unique<antenna::PythonAntenna>(
				antName,
				getAttributeString(antXml, "module", "Python antenna must specify a module"),
				getAttributeString(antXml, "function", "Python antenna must specify a function")
			);
		}
		if (antPattern == "sinc")
		{
			return std::make_unique<antenna::Sinc>(
				antName,
				getChildRsFloat(antXml, "alpha"),
				getChildRsFloat(antXml, "beta"),
				getChildRsFloat(antXml, "gamma")
			);
		}
		if (antPattern == "gaussian")
		{
			return std::make_unique<antenna::Gaussian>(
				antName,
				getChildRsFloat(antXml, "azscale"),
				getChildRsFloat(antXml, "elscale")
			);
		}
		if (antPattern == "parabolic")
		{
			return std::make_unique<antenna::ParabolicReflector>(antName, getChildRsFloat(antXml, "diameter"));
		}

		// Return nullptr if no valid antenna type matches
		return nullptr;
	}

	void processAntenna(const TiXmlHandle& antXml, World* world)
	{
		const std::string ant_name = getAttributeString(antXml, "name", "Antennas must specify a name");
		const std::string ant_pattern = getAttributeString(antXml, "pattern", "Antennas must specify a pattern");

		// Use a factory method to create the appropriate antenna
		auto antenna = createAntenna(ant_pattern, antXml, ant_name);
		if (!antenna) { throw XmlImportException("Antenna specified unrecognised gain pattern '" + ant_pattern + "'"); }

		LOG(Level::DEBUG, "Loading antenna '{}' of type '{}'", ant_name.c_str(),
		    ant_pattern.c_str());

		try { antenna->setEfficiencyFactor(getChildRsFloat(antXml, "efficiency")); }
		catch (XmlImportException&)
		{
			LOG(Level::DEBUG,
			    "Antenna '{}' does not specify efficiency, assuming unity.", ant_name.c_str());
		}

		world->add(std::move(antenna));
	}

	void processMultipath(const TiXmlHandle& mpXml, World* world)
	{
		auto mps = std::make_unique<math::MultipathSurface>(
			getChildRsFloat(mpXml, "nx"),
			getChildRsFloat(mpXml, "ny"),
			getChildRsFloat(mpXml, "nz"),
			getChildRsFloat(mpXml, "d"),
			getChildRsFloat(mpXml, "factor")
		);
		world->addMultipathSurface(std::move(mps));
	}

	/// Process a timing source and add it to the world
	void processTiming(const TiXmlHandle& antXml, World* world)
	{
		const std::string name = getAttributeString(antXml, "name", "Timing sources must specify a name");
		auto timing = std::make_unique<PrototypeTiming>(name);

		int i{};
		for (TiXmlHandle plat = antXml.ChildElement("noise_entry", 0); plat.Element(); plat = antXml.
		     ChildElement("noise_entry", ++i))
		{
			timing->setAlpha(getChildRsFloat(plat, "alpha"), getChildRsFloat(plat, "weight"));
		}

		try { timing->setFreqOffset(getChildRsFloat(antXml, "freq_offset")); }
		catch (XmlImportException&)
		{
			LOG(Level::DEBUG, "Clock section '{}' does not specify frequency offset.",
			    name.c_str());
		}

		try { timing->setRandomFreqOffset(getChildRsFloat(antXml, "random_freq_offset")); }
		catch (XmlImportException&)
		{
			LOG(Level::DEBUG,
			    "Clock section '{}' does not specify random frequency offset.", name.c_str());
		}

		try { timing->setPhaseOffset(getChildRsFloat(antXml, "phase_offset")); }
		catch (XmlImportException&)
		{
			LOG(Level::DEBUG, "Clock section '{}' does not specify phase offset.",
			    name.c_str());
		}

		try { timing->setRandomPhaseOffset(getChildRsFloat(antXml, "random_phase_offset")); }
		catch (XmlImportException&)
		{
			LOG(Level::DEBUG, "Clock section '{}' does not specify random phase offset.",
			    name.c_str());
		}

		try { timing->setFrequency(getChildRsFloat(antXml, "frequency")); }
		catch (XmlImportException&)
		{
			timing->setFrequency(params::rate());
			LOG(Level::DEBUG,
			    "Clock section '{}' does not specify frequency. Assuming {}.", name.c_str(),
			    params::rate());
		}

		if (getAttributeBool(antXml, "synconpulse", "", true)) { timing->setSyncOnPulse(); }

		LOG(Level::DEBUG, "Loading timing source '{}'", name.c_str());
		world->add(std::move(timing));
	}

	/// Process the <parameters> element
	void processParameters(const TiXmlHandle& root)
	{
		params::setTime(getChildRsFloat(root, "starttime"), getChildRsFloat(root, "endtime"));

		try { params::setC(getChildRsFloat(root, "c")); }
		catch (XmlImportException&) { LOG(Level::DEBUG, "Using default value of c: {}(m/s)", params::c()); }

		try { params::setRate(getChildRsFloat(root, "rate")); }
		catch (XmlImportException&) { LOG(Level::DEBUG, "Using default sampling rate."); }

		try { params::setCwSampleRate(getChildRsFloat(root, "interprate")); }
		catch (XmlImportException&)
		{
			LOG(Level::DEBUG,
			    "Using default value of CW position interpolation rate: {}",
			    params::cwSampleRate());
		}

		try { params::setRandomSeed(static_cast<unsigned>(std::fabs(getChildRsFloat(root, "randomseed")))); }
		catch (XmlImportException&)
		{
			LOG(Level::DEBUG, "Using random seed from clock(): {}",
			    params::randomSeed());
		}

		try
		{
			params::setAdcBits(static_cast<unsigned>(std::floor(getChildRsFloat(root, "adc_bits"))));
			LOG(Level::DEBUG, "Quantizing results to {} bits", params::adcBits());
		}
		catch (XmlImportException&) { LOG(Level::DEBUG, "Using full precision simulation."); }

		try { params::setOversampleRatio(static_cast<unsigned>(std::floor(getChildRsFloat(root, "oversample")))); }
		catch (XmlImportException&)
		{
			LOG(Level::DEBUG,
			    "Oversampling not in use. Ensure than pulses are correctly sampled.");
		}

		if (const TiXmlHandle exporttag = root.ChildElement("export", 0); exporttag.Element())
		{
			params::setExporters(
				getAttributeBool(exporttag, "xml", "", params::exportXml()),
				getAttributeBool(exporttag, "csv", "", params::exportCsv()),
				getAttributeBool(exporttag, "binary", "", params::exportBinary())
			);
		}
	}

	/// Process the XML tree, starting at the root
	void processDocument(const TiXmlHandle& root, World* world, bool included);

	/// Process the inclusion of a file
	void processInclude(const TiXmlHandle& plat, World* world) // NOLINT(misc-no-recursion)
	{
		TiXmlDocument doc(plat.Element()->GetText());
		if (!doc.LoadFile())
		{
			throw std::runtime_error("Cannot open included file: " + std::string(plat.Element()->GetText()));
		}
		processDocument(TiXmlHandle(doc.RootElement()), world, true);
	}

	/// Process an element in the XML tree
	void processElement(const TiXmlHandle& root, const std::string& element, World* world,
	                    const std::function<void(const TiXmlHandle&, World*)>& processFunc)
	{
		for (TiXmlHandle handle = root.ChildElement(element.c_str(), 0); handle.Element();
		     handle = handle.Element()->NextSiblingElement(element.c_str())) { processFunc(handle, world); }
	}

	/// Process the XML tree, starting at the root
	void processDocument(const TiXmlHandle& root, World* world, const bool included) // NOLINT(misc-no-recursion)
	{
		if (!included) { processParameters(root.ChildElement("parameters", 0)); }

		processElement(root, "pulse", world, processPulse);
		processElement(root, "antenna", world, processAntenna);
		processElement(root, "timing", world, processTiming);
		processElement(root, "multipath", world, processMultipath);
		processElement(root, "platform", world, processPlatform);
		processElement(root, "include", world, processInclude);

		// For "incblock", we need to recursively call processDocument with 'included = true'
		processElement(root, "incblock", world, [](const TiXmlHandle& handle, World* world2)
		{
			processDocument(handle, world2, true);
		});
	}
} //Anonymous Namespace

// =====================================================================================================================
//
// XML IMPORT FUNCTION
//
// =====================================================================================================================

namespace serial
{
	/// Load an XML file into the world with the given filename
	void loadXmlFile(const std::string& filename, World* world)
	{
		TiXmlDocument doc(filename.c_str());
		if (!doc.LoadFile()) { throw std::runtime_error("Cannot open script file"); }
		processDocument(TiXmlHandle(doc.RootElement()), world, false);
		world->processMultipath();
	}
}
