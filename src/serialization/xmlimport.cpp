// xmlimport.cpp
// Import a simulator world and simulation parameters from an XML file
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started 26 April 2006

#define TIXML_USE_STL

#include "xmlimport.h"

#include <functional>
#include <optional>
#include <tinyxml.h>

#include "pulse_factory.h"
#include "core/parameters.h"
#include "math_utils/multipath_surface.h"
#include "radar/radar_system.h"
#include "radar/target.h"

using namespace rs;

//
// XML Parsing Utility Functions
//

/// Exception for reporting an XML parsing error
class XmlImportException final : public std::runtime_error
{
public:
	explicit XmlImportException(const std::string& error):
		std::runtime_error("[ERROR] Error while parsing XML file: " + error) {}
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
RS_FLOAT getChildRsFloat(const TiXmlHandle& parent, const char* childname)
{
	const char* data = getChildText(parent, childname);
	if (!data)
	{
		throw XmlImportException("No data in child element " + std::string(childname) + " during GetChildRsFloat.");
	}
	RS_FLOAT result;
	std::istringstream(data) >> result;
	return result;
}

/// Gets the text content of a node as an rsFloat.
//For XML like this:
//<rcs>10</rcs>
RS_FLOAT getNodeFloat(const TiXmlHandle& node)
{
	const char* data = node.Element() ? node.Element()->GetText() : nullptr;
	if (!data) { throw XmlImportException("Node does not contain text during GetNodeFloat"); }
	RS_FLOAT result;
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
	void processTarget(const TiXmlHandle& targXml, const Platform* platform, World* world)
	{
		const std::string name = getAttributeString(targXml, "name", "Target does not specify a name");
		const TiXmlHandle rcs_xml = targXml.ChildElement("rcs", 0);
		if (!rcs_xml.Element()) { throw XmlImportException("Target " + name + " does not specify RCS."); }

		const std::string rcs_type = getAttributeString(rcs_xml, "type",
		                                                "RCS attached to target '" + name + "' does not specify type.");
		Target* target = nullptr;

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
			if (model_type == "constant") { target->setFluctuationModel(new RcsConst()); }
			else if (model_type == "chisquare" || model_type == "gamma")
			{
				target->setFluctuationModel(new RcsChiSquare(getChildRsFloat(model_xml, "k")));
			}
			else { throw XmlImportException("Target fluctuation model type '" + model_type + "' not recognised."); }
		}

		world->add(target);
	}

	/// Process a receiver XML entry
	Receiver* processReceiver(const TiXmlHandle& recvXml, const Platform* platform, World* world)
	{
		logging::printf(logging::RS_VERY_VERBOSE, "[VV] Loading Receiver: ");

		const std::string name = getAttributeString(recvXml, "name", "Receiver does not specify a name");
		auto* receiver = new Receiver(platform, name);

		const std::string ant_name = getAttributeString(recvXml, "antenna",
		                                                "Receiver '" + name + "' does not specify an antenna");
		logging::printf(logging::RS_VERY_VERBOSE, "'%s'\n", receiver->getName().c_str());

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
			logging::printf(logging::RS_VERBOSE, "[WARNING] Receiver '%s' does not specify noise temperature.\n",
			                receiver->getName().c_str());
		}

		receiver->setWindowProperties(
			getChildRsFloat(recvXml, "window_length"),
			getChildRsFloat(recvXml, "prf"),
			getChildRsFloat(recvXml, "window_skip")
		);

		const std::string timing_name = getAttributeString(recvXml, "timing",
		                                                   "Receiver '" + name + "' does not specify a timing source");
		auto* timing = new ClockModelTiming(timing_name);

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
			receiver->setFlag(Receiver::FLAG_NODIRECT);
			logging::printf(logging::RS_VERY_VERBOSE, "[VV] Ignoring direct signals for receiver '%s'\n",
			                receiver->getName().c_str());
		}

		if (getAttributeBool(recvXml, "nopropagationloss", "", false))
		{
			receiver->setFlag(Receiver::FLAG_NOPROPLOSS);
			logging::printf(logging::RS_VERY_VERBOSE, "[VV] Ignoring propagation losses for receiver '%s'\n",
			                receiver->getName().c_str());
		}

		world->add(receiver);
		return receiver;
	}

	/// Create a PulseTransmitter object and process XML entry
	Transmitter* processPulseTransmitter(const TiXmlHandle& transXml, const std::string& name, const Platform* platform,
	                                     World* world)
	{
		auto* transmitter = new Transmitter(platform, name, true);
		const std::string pulse_name = getAttributeString(transXml, "pulse",
		                                                  "Transmitter '" + name + "' does not specify a pulse");
		RadarSignal* wave = world->findSignal(pulse_name);
		if (!wave) { throw XmlImportException("Pulse with name '" + pulse_name + "' does not exist"); }
		transmitter->setWave(wave);
		transmitter->setPrf(getChildRsFloat(transXml, "prf"));
		return transmitter;
	}

	/// Create a PulseTransmitter object and process XML entry
	Transmitter* processCwTransmitter(const TiXmlHandle& transXml, const std::string& name, const Platform* platform,
	                                  World* world)
	{
		auto* transmitter = new Transmitter(platform, name, false);
		const std::string pulse_name = getAttributeString(transXml, "pulse",
		                                                  "Transmitter '" + name + "' does not specify a pulse");
		RadarSignal* wave = world->findSignal(pulse_name);
		if (!wave) { throw XmlImportException("Pulse with name '" + pulse_name + "' does not exist"); }
		transmitter->setWave(wave);
		return transmitter;
	}

	/// Process a transmitter XML entry
	std::optional<Transmitter*> processTransmitter(const TiXmlHandle& transXml, const Platform* platform, World* world,
	                                               const bool isMonostatic = false)
	{
		// David Young: We use C++17's std::optional to indicate that the function may return a null value.
		// This allows the World object to manage the deletion of the Transmitter object,
		// since the return value of this function is ignored in the bistatic case
		// (as handled by processPlatform).
		logging::printf(logging::RS_VERY_VERBOSE, "[VV] Loading Transmitter: ");

		const std::string name = getAttributeString(transXml, "name", "Transmitter does not specify a name");
		const std::string type = getAttributeString(transXml, "type",
		                                            "Transmitter '" + name + "' does not specify type");

		Transmitter* transmitter = type == "pulsed"
			                           ? processPulseTransmitter(transXml, name, platform, world)
			                           : type == "continuous"
			                           ? processCwTransmitter(transXml, name, platform, world)
			                           : throw XmlImportException(
				                           "[ERROR] Invalid transmitter type specified in transmitter " + name);

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
		auto* timing = new ClockModelTiming(name);
		const PrototypeTiming* proto = world->findTiming(timing_name);
		if (!proto)
		{
			throw XmlImportException(
				"Timing source '" + timing_name + "' does not exist when processing receiver " + name);
		}
		timing->initializeModel(proto);
		transmitter->setTiming(timing);

		world->add(transmitter);

		return isMonostatic ? std::optional(transmitter) : std::nullopt;
	}

	/// Process a monostatic (Receiver and Transmitter sharing an antenna)
	void processMonostatic(const TiXmlHandle& transXml, const Platform* platform, World* world)
	{
		Transmitter* trans = processTransmitter(transXml, platform, world, true).value();
		Receiver* recv = processReceiver(transXml, platform, world);
		trans->makeMonostatic(recv);
		recv->makeMonostatic(trans);
	}

	/// Process a motion path waypoint
	void processWaypoint(const TiXmlHandle& handXml, path::Path* path)
	{
		try
		{
			const RS_FLOAT x = getChildRsFloat(handXml, "x");
			const RS_FLOAT y = getChildRsFloat(handXml, "y");
			const RS_FLOAT z = getChildRsFloat(handXml, "altitude");
			const RS_FLOAT t = getChildRsFloat(handXml, "time");
			coord::Coord coord;
			coord.t = t;
			coord.pos = Vec3(x, y, z);
			path->addCoord(coord);
		}
		catch (XmlImportException& e)
		{
			logging::printf(logging::RS_VERBOSE,
			                "[WARNING] Parse Error While Importing Waypoint. Discarding Waypoint.\n%s", e.what());
		}
	}

	/// Process the path's python attributes
	void processPythonPath(const TiXmlHandle& pathXml, path::Path* path)
	{
		rs_python::initPython();
		try
		{
			const TiXmlHandle tmp = pathXml.FirstChildElement("pythonpath");
			path->loadPythonPath(
				getAttributeString(tmp, "module", "Attribute module missing"),
				getAttributeString(tmp, "function", "Attribute function missing")
			);
		}
		catch (const XmlImportException& e) { logging::printf(logging::RS_VERBOSE, "%s", e.what()); }
	}

	/// Process a MotionPath XML entry
	void processMotionPath(const TiXmlHandle& mpXml, const Platform* platform)
	{
		path::Path* path = platform->getMotionPath();

		try
		{
			if (const std::string rottype = getAttributeString(mpXml, "interpolation", ""); rottype == "linear")
			{
				path->setInterp(path::Path::RS_INTERP_LINEAR);
			}
			else if (rottype == "cubic") { path->setInterp(path::Path::RS_INTERP_CUBIC); }
			else if (rottype == "static") { path->setInterp(path::Path::RS_INTERP_STATIC); }
			else if (rottype == "python")
			{
				path->setInterp(path::Path::RS_INTERP_PYTHON);
				processPythonPath(mpXml, path);
			}
			else
			{
				logging::printf(logging::RS_VERBOSE,
				                "[WARNING] Unsupported motion path interpolation type for platform '%s'. Defaulting to static.\n",
				                platform->getName().c_str());
				path->setInterp(path::Path::RS_INTERP_STATIC);
			}
		}
		catch (XmlImportException&)
		{
			logging::printf(logging::RS_VERBOSE,
			                "[WARNING] Motion path interpolation type not specified for platform '%s'. Defaulting to static.\n",
			                platform->getName().c_str());
			path->setInterp(path::Path::RS_INTERP_STATIC);
		}

		int i{};
		for (TiXmlHandle tmp = mpXml.ChildElement("positionwaypoint", 0); tmp.Element();
		     tmp = mpXml.ChildElement("positionwaypoint", ++i)) { processWaypoint(tmp, path); }

		path->finalize();
	}

	/// Process a rotation path waypoint
	void processRotationWaypoint(const TiXmlHandle& handXml, path::RotationPath* path)
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
			logging::printf(logging::RS_VERBOSE,
			                "[WARNING] Parse Error While Importing Waypoint. Discarding Waypoint.\n%s", e.what());
		}
	}

	/// Process Waypoints for RotationPath
	void processRotationWaypoints(const TiXmlHandle& mpXml, path::RotationPath* path)
	{
		for (TiXmlHandle tmp = mpXml.ChildElement("rotationwaypoint", 0); tmp.Element(); tmp = tmp.Element()->
		     NextSiblingElement("rotationwaypoint")) { processRotationWaypoint(tmp, path); }
		path->finalize();
	}

	/// Process an entry for a fixed rotation
	void processRotationConstant(const TiXmlHandle& mpXml, const Platform* platform)
	{
		path::RotationPath* path = platform->getRotationPath();
		try
		{
			coord::RotationCoord start, rate;
			start.azimuth = getChildRsFloat(mpXml, "startazimuth");
			start.elevation = getChildRsFloat(mpXml, "startelevation");
			rate.azimuth = getChildRsFloat(mpXml, "azimuthrate");
			rate.elevation = getChildRsFloat(mpXml, "elevationrate");
			path->setConstantRate(start, rate);
		}
		catch (XmlImportException& e)
		{
			logging::printf(logging::RS_VERBOSE, "[WARNING] Parse Error While Importing Constant Rotation.\n%s",
			                e.what());
		}
	}

	/// Process a RotationPath XML entry
	void processRotationPath(const TiXmlHandle& mpXml, const Platform* platform)
	{
		logging::printf(logging::RS_VERY_VERBOSE, "[VV] Loading Rotation Path.\n");

		path::RotationPath* path = platform->getRotationPath();

		try
		{
			if (const std::string rottype = getAttributeString(mpXml, "interpolation", ""); rottype == "linear")
			{
				path->setInterp(path::RotationPath::RS_INTERP_LINEAR);
			}
			else if (rottype == "cubic") { path->setInterp(path::RotationPath::RS_INTERP_CUBIC); }
			else if (rottype == "static") { path->setInterp(path::RotationPath::RS_INTERP_STATIC); }
			else
			{
				logging::printf(logging::RS_VERBOSE,
				                "[WARNING] Unsupported rotation path interpolation type for platform '%s'. Defaulting to static.\n",
				                platform->getName().c_str());
				path->setInterp(path::RotationPath::RS_INTERP_STATIC);
			}
		}
		catch (XmlImportException&)
		{
			logging::printf(logging::RS_VERBOSE,
			                "[WARNING] Rotation path interpolation type not specified for platform '%s'. Defaulting to static.\n",
			                platform->getName().c_str());
			path->setInterp(path::RotationPath::RS_INTERP_STATIC);
		}

		processRotationWaypoints(mpXml, path);
	}

	/// Process a platform, recursively processing all the elements that are attached to it
	void processPlatform(const TiXmlHandle& platXml, World* world)
	{
		const std::string name = getAttributeString(platXml, "name", "[ERROR] Platform must specify a name");
		auto* platform = new Platform(name);
		world->add(platform);

		int i{};
		for (TiXmlHandle tmp = platXml.ChildElement("target", 0); tmp.Element();
		     tmp = platXml.ChildElement("target", ++i)) { processTarget(tmp, platform, world); }

		i = 0;
		for (TiXmlHandle tmp = platXml.ChildElement("receiver", 0); tmp.Element();
		     tmp = platXml.ChildElement("receiver", ++i)) { processReceiver(tmp, platform, world); }

		i = 0;
		for (TiXmlHandle tmp = platXml.ChildElement("transmitter", 0); tmp.Element();
		     tmp = platXml.ChildElement("transmitter", ++i)) { processTransmitter(tmp, platform, world); }

		i = 0;
		for (TiXmlHandle tmp = platXml.ChildElement("monostatic", 0); tmp.Element();
		     tmp = platXml.ChildElement("monostatic", ++i)) { processMonostatic(tmp, platform, world); }

		i = 0;
		for (TiXmlHandle tmp = platXml.ChildElement("motionpath", 0); tmp.Element();
		     tmp = platXml.ChildElement("motionpath", ++i)) { processMotionPath(tmp, platform); }

		i = 0;
		for (TiXmlHandle tmp = platXml.ChildElement("rotationpath", 0); tmp.Element();
		     tmp = platXml.ChildElement("rotationpath", ++i)) { processRotationPath(tmp, platform); }

		i = 0;
		for (TiXmlHandle tmp = platXml.ChildElement("fixedrotation", 0); tmp.Element();
		     tmp = platXml.ChildElement("fixedrotation", ++i)) { processRotationConstant(tmp, platform); }
	}

	/// Process a pulse entry of type rect
	void processAnyPulseFile(const TiXmlHandle& pulseXml, World* world, const std::string& name)
	{
		RadarSignal* wave = pulse_factory::loadPulseFromFile(
			name,
			getAttributeString(pulseXml, "filename", "Pulse must specify a filename"),
			getChildRsFloat(pulseXml, "power"),
			getChildRsFloat(pulseXml, "carrier")
		);
		world->add(wave);
	}

	/// Process a pulse entry
	void processPulse(const TiXmlHandle& pulseXml, World* world)
	{
		const std::string pulse_name = getAttributeString(pulseXml, "name", "Pulses must specify a name");
		const std::string pulse_type = getAttributeString(pulseXml, "type", "Pulses must specify a type");
		logging::printf(logging::RS_VERY_VERBOSE, "[VV] Generating Pulse %s of type '%s'\n", pulse_name.c_str(),
		                pulse_type.c_str());

		if (pulse_type == "file") { processAnyPulseFile(pulseXml, world, pulse_name); }
		else { throw XmlImportException("Unrecognised type in pulse"); }
	}

	Antenna* processPythonAntenna(const TiXmlHandle& antXml, const std::string& name)
	{
		rs_python::initPython();
		return createPythonAntenna(
			name,
			getAttributeString(antXml, "module", "Attribute module missing"),
			getAttributeString(antXml, "function", "Attribute function missing")
		);
	}

	Antenna* processXmlAntenna(const TiXmlHandle& antXml, const std::string& name)
	{
		return createXmlAntenna(
			name, getAttributeString(antXml, "filename", "Antenna definition must specify a filename"));
	}

	Antenna* processFileAntenna(const TiXmlHandle& antXml, const std::string& name)
	{
		return createFileAntenna(
			name, getAttributeString(antXml, "filename", "Antenna definition must specify a filename"));
	}

	Antenna* processSincAntenna(const TiXmlHandle& antXml, const std::string& name)
	{
		return createSincAntenna(
			name,
			getChildRsFloat(antXml, "alpha"),
			getChildRsFloat(antXml, "beta"),
			getChildRsFloat(antXml, "gamma")
		);
	}

	Antenna* processGaussianAntenna(const TiXmlHandle& antXml, const std::string& name)
	{
		return createGaussianAntenna(
			name,
			getChildRsFloat(antXml, "azscale"),
			getChildRsFloat(antXml, "elscale")
		);
	}

	Antenna* processParabolicAntenna(const TiXmlHandle& antXml, const std::string& name)
	{
		return createParabolicAntenna(name, getChildRsFloat(antXml, "diameter"));
	}

	void processAntenna(const TiXmlHandle& antXml, World* world)
	{
		const std::string ant_name = getAttributeString(antXml, "name", "Antennas must specify a name");
		const std::string ant_pattern = getAttributeString(antXml, "pattern", "Antennas must specify a pattern");
		Antenna* antenna;

		if (ant_pattern == "isotropic") { antenna = createIsotropicAntenna(ant_name); }
		else if (ant_pattern == "file") { antenna = processFileAntenna(antXml, ant_name); }
		else if (ant_pattern == "xml") { antenna = processXmlAntenna(antXml, ant_name); }
		else if (ant_pattern == "python") { antenna = processPythonAntenna(antXml, ant_name); }
		else if (ant_pattern == "sinc") { antenna = processSincAntenna(antXml, ant_name); }
		else if (ant_pattern == "gaussian") { antenna = processGaussianAntenna(antXml, ant_name); }
		else if (ant_pattern == "parabolic") { antenna = processParabolicAntenna(antXml, ant_name); }
		else { throw XmlImportException("Antenna specified unrecognised gain pattern '" + ant_pattern + "'"); }

		logging::printf(logging::RS_VERY_VERBOSE, "[VV] Loading antenna '%s' of type '%s'\n", ant_name.c_str(),
		                ant_pattern.c_str());

		try { antenna->setEfficiencyFactor(getChildRsFloat(antXml, "efficiency")); }
		catch (XmlImportException&)
		{
			logging::printf(logging::RS_VERBOSE,
			                "[VERBOSE] Antenna '%s' does not specify efficiency, assuming unity.\n", ant_name.c_str());
		}

		world->add(antenna);
	}

	void processMultipath(const TiXmlHandle& mpXml, World* world)
	{
		auto* mps = new MultipathSurface(
			getChildRsFloat(mpXml, "nx"),
			getChildRsFloat(mpXml, "ny"),
			getChildRsFloat(mpXml, "nz"),
			getChildRsFloat(mpXml, "d"),
			getChildRsFloat(mpXml, "factor")
		);
		world->addMultipathSurface(mps);
	}

	/// Process a timing source and add it to the world
	void processTiming(const TiXmlHandle& antXml, World* world)
	{
		const std::string name = getAttributeString(antXml, "name", "Timing sources must specify a name");
		auto* timing = new PrototypeTiming(name);

		int i{};
		for (TiXmlHandle plat = antXml.ChildElement("noise_entry", 0); plat.Element(); plat = antXml.
		     ChildElement("noise_entry", ++i))
		{
			timing->addAlpha(getChildRsFloat(plat, "alpha"), getChildRsFloat(plat, "weight"));
		}

		try { timing->addFreqOffset(getChildRsFloat(antXml, "freq_offset")); }
		catch (XmlImportException&)
		{
			logging::printf(logging::RS_VERY_VERBOSE, "[VV] Clock section '%s' does not specify frequency offset.\n",
			                name.c_str());
		}

		try { timing->addRandomFreqOffset(getChildRsFloat(antXml, "random_freq_offset")); }
		catch (XmlImportException&)
		{
			logging::printf(logging::RS_VERY_VERBOSE,
			                "[VV] Clock section '%s' does not specify random frequency offset.\n", name.c_str());
		}

		try { timing->addPhaseOffset(getChildRsFloat(antXml, "phase_offset")); }
		catch (XmlImportException&)
		{
			logging::printf(logging::RS_VERY_VERBOSE, "[VV] Clock section '%s' does not specify phase offset.\n",
			                name.c_str());
		}

		try { timing->addRandomPhaseOffset(getChildRsFloat(antXml, "random_phase_offset")); }
		catch (XmlImportException&)
		{
			logging::printf(logging::RS_VERY_VERBOSE, "[VV] Clock section '%s' does not specify random phase offset.\n",
			                name.c_str());
		}

		try { timing->setFrequency(getChildRsFloat(antXml, "frequency")); }
		catch (XmlImportException&)
		{
			timing->setFrequency(parameters::rate());
			logging::printf(logging::RS_VERBOSE,
			                "[VERBOSE] Clock section '%s' does not specify frequency. Assuming %g.\n", name.c_str(),
			                parameters::rate());
		}

		if (getAttributeBool(antXml, "synconpulse", "", true)) { timing->setSyncOnPulse(); }

		logging::printf(logging::RS_VERY_VERBOSE, "[VV] Loading timing source '%s'\n", name.c_str());
		world->add(timing);
	}

	/// Process the <parameters> element
	void processParameters(const TiXmlHandle& root)
	{
		parameters::setTime(getChildRsFloat(root, "starttime"), getChildRsFloat(root, "endtime"));

		try { parameters::setC(getChildRsFloat(root, "c")); }
		catch (XmlImportException&)
		{
			logging::printf(logging::RS_VERBOSE, "[VERBOSE] Using default value of c: %f(m/s)\n", parameters::c());
		}

		try { parameters::setRate(getChildRsFloat(root, "rate")); }
		catch (XmlImportException&)
		{
			logging::printf(logging::RS_VERBOSE, "[VERBOSE] Using default sampling rate.\n");
		}

		try { parameters::setCwSampleRate(getChildRsFloat(root, "interprate")); }
		catch (XmlImportException&)
		{
			logging::printf(logging::RS_VERBOSE,
			                "[VERBOSE] Using default value of CW position interpolation rate: %g\n",
			                parameters::cwSampleRate());
		}

		try { parameters::setRandomSeed(static_cast<unsigned>(std::fabs(getChildRsFloat(root, "randomseed")))); }
		catch (XmlImportException&)
		{
			logging::printf(logging::RS_VERBOSE, "[VERBOSE] Using random seed from clock(): %d\n",
			                parameters::randomSeed());
		}

		try
		{
			parameters::setAdcBits(static_cast<unsigned>(std::floor(getChildRsFloat(root, "adc_bits"))));
			logging::printf(logging::RS_VERBOSE, "[VERBOSE] Quantizing results to %d bits\n", parameters::adcBits());
		}
		catch (XmlImportException&)
		{
			logging::printf(logging::RS_VERY_VERBOSE, "[VERBOSE] Using full precision simulation.\n");
		}

		try { parameters::setOversampleRatio(static_cast<unsigned>(std::floor(getChildRsFloat(root, "oversample")))); }
		catch (XmlImportException&)
		{
			logging::printf(logging::RS_VERY_VERBOSE,
			                "[VV] Oversampling not in use. Ensure than pulses are correctly sampled.\n");
		}

		if (const TiXmlHandle exporttag = root.ChildElement("export", 0); exporttag.Element())
		{
			parameters::setExporters(
				getAttributeBool(exporttag, "xml", "", parameters::exportXml()),
				getAttributeBool(exporttag, "csv", "", parameters::exportCsv()),
				getAttributeBool(exporttag, "binary", "", parameters::exportBinary())
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

/// Load an XML file into the world with the given filename
void xml::loadXmlFile(const std::string& filename, World* world)
{
	TiXmlDocument doc(filename.c_str());
	if (!doc.LoadFile()) { throw std::runtime_error("Cannot open script file"); }
	processDocument(TiXmlHandle(doc.RootElement()), world, false);
	world->processMultipath();
}
