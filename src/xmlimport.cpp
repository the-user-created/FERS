//xmlimport.cpp
//Import a simulator world and simulation parameters from an XML file
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started 26 April 2006

//TODO: Rewrite this code to be less ugly

#define TIXML_USE_STL //Tell tinyxml to use the STL instead of it's own string class

#include "xmlimport.h"

#include <cmath>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tinyxml.h>

#include "rsantenna.h"
#include "rsdebug.h"
#include "rsmultipath.h"
#include "rsparameters.h"
#include "rsplatform.h"
#include "rspython.h"
#include "rsradar.h"
#include "rstarget.h"
#include "rstiming.h"
#include "rsworld.h"

using namespace rs;
using std::string;

//
// XML Parsing Utility Functions
//

/// Exception for reporting an XML parsing error
class XmlImportException final : public std::runtime_error
{
public:
	explicit XmlImportException(const std::string& error):
		std::runtime_error("[ERROR] Error while parsing XML file: " + error)
	{
	}
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
	const TiXmlHandle tmp = parent.FirstChildElement(childname);
	if (tmp.Element() == nullptr)
	{
		return nullptr; //The element does not exist
	}
	//Return the text
	return tmp.Element()->GetText();
}

/// Gets child text as a rsFloat. See GetChildText for usage description
RS_FLOAT getChildRsFloat(const TiXmlHandle& parent, const char* childname)
{
	const char* data = getChildText(parent, childname);
	//If there is no data, flag failure and return
	if (!data)
	{
		throw XmlImportException("No data in child element " + string(childname) + " during GetChildRsFloat.");
	}
	//Parse the first rsFloat from the data
	RS_FLOAT result;
	std::istringstream iss(data);
	iss >> result;
	return result;
}

/// Get the text contents of a node
const char* getNodeText(const TiXmlHandle& parent)
{
	//Return the text
	return parent.Element()->GetText();
}

/// Gets the text content of a node as an rsFloat.
//For XML like this:
//<rcs>10</rcs>
RS_FLOAT getNodeFloat(const TiXmlHandle& node)
{
	if (!node.Element())
	{
		throw XmlImportException("[BUG] Node does not exist during GetNodeFloat");
	}
	const char* data = node.Element()->GetText();
	if (!data)
	{
		throw XmlImportException("Node does not contain text during GetNodeFloat");
	}
	RS_FLOAT result;
	std::istringstream iss(data);
	iss >> result;
	return result;
}

/// Return the string associated with an attribute or throw an exception on failure
std::string getAttributeString(const TiXmlHandle& handle, const std::string& name, const std::string& error,
                               const bool optional = false)
{
	if (const std::string* text = handle.Element()->Attribute(name))
	{
		return *text;
	}
	if (!optional)
	{
		throw XmlImportException(error);
	}
	return {};
}

/// Return the bool associated with an attribute
bool getAttributeBool(const TiXmlHandle& handle, const std::string& name, const std::string& error, const bool def,
                      const bool optional = true)
{
	const string str = getAttributeString(handle, name, error, optional);
	if (str == "")
	{
		return def;
	}
	return str == "true" || str == "yes";
}


namespace
{
	///Process a Gamma target model entry
	RcsModel* processGammaModel(const TiXmlHandle& modelXml)
	{
		const RS_FLOAT k = getChildRsFloat(modelXml, "k");
		return new RcsChiSquare(k);
	}

	/// Process a target XML entry
	void processTarget(const TiXmlHandle& targXml, const Platform* platform, World* world)
	{
		Target* target;
		const string name = getAttributeString(targXml, "name", "Target does not specify a name");
		//Get the RCS
		const TiXmlHandle rcs_xml = targXml.ChildElement("rcs", 0);
		if (!rcs_xml.Element())
		{
			throw XmlImportException("Target " + name + " does not specify RCS.");
		}
		const string rcs_type = getAttributeString(rcs_xml, "type",
		                                           "RCS attached to target '" + name + "' does not specify type.");
		// Handle the target type (isotropic, file imported, etc.)
		if (rcs_type == "isotropic")
		{
			const TiXmlHandle rcs_value_xml = rcs_xml.ChildElement("value", 0);
			if (!rcs_value_xml.Element())
			{
				throw XmlImportException("Target " + name + " does not specify value of isotropic RCS.");
			}
			const RS_FLOAT value = getNodeFloat(rcs_value_xml);
			target = createIsoTarget(platform, name, value);
		}
		else if (rcs_type == "file")
		{
			const string filename = getAttributeString(rcs_xml, "filename",
			                                           "RCS attached to target '" + name +
			                                           "' does not specify filename.");
			target = createFileTarget(platform, name, filename);
		}
		else
		{
			throw XmlImportException("RCS type " + rcs_type + " not currently supported.");
		}
		//Handle the target statistical model
		if (const TiXmlHandle model_xml = targXml.ChildElement("model", 0); model_xml.Element())
		{
			//Get the mode type
			const string model_type = getAttributeString(model_xml, "type",
			                                             "Model attached to target '" + name +
			                                             "' does not specify type.");
			if (model_type == "constant")
			{
				auto* model = new RcsConst();
				target->setFluctuationModel(model);
			}
			else if (model_type == "chisquare" || model_type == "gamma")
			{
				RcsModel* model = processGammaModel(model_xml);
				target->setFluctuationModel(model);
			}
			else
			{
				throw XmlImportException("Target fluctuation model type '" + model_type + "' not recognised.");
			}
		}
		//Add the target to the world
		world->add(target);
	}

	/// Process a receiver XML entry
	Receiver* processReceiver(const TiXmlHandle& recvXml, const Platform* platform, World* world)
	{
		rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Loading Receiver: ");

		//Get the name of the receiver
		string name = getAttributeString(recvXml, "name", "Receiver does not specify a name");
		auto* receiver = new Receiver(platform, name);

		//Get the name of the antenna
		const string ant_name = getAttributeString(recvXml, "antenna",
		                                           "Receiver '" + string(name) + "' does not specify an antenna");

		rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "'%s' ", receiver->getName().c_str());

		const Antenna* antenna = world->findAntenna(ant_name);
		if (!antenna)
		{
			throw XmlImportException(
				"Antenna with name '" + ant_name + "' does not exist when processing Receiver " + string(name));
		}
		//Set the receiver's antenna
		receiver->setAntenna(antenna);

		//Process the noise temperature tag
		try
		{
			const RS_FLOAT temperature = getChildRsFloat(recvXml, "noise_temp");
			receiver->setNoiseTemperature(temperature);
		}
		catch ([[maybe_unused]] XmlImportException& e)
		{
		}

		//Process the PRF tag
		const RS_FLOAT prf = getChildRsFloat(recvXml, "prf");
		const RS_FLOAT skip = getChildRsFloat(recvXml, "window_skip");
		const RS_FLOAT length = getChildRsFloat(recvXml, "window_length");
		receiver->setWindowProperties(length, prf, skip);

		//Get the name of the timing source
		const string timing_name = getAttributeString(recvXml, "timing",
		                                              "Receiver '" + name + "' does not specify a timing source");
		auto* timing = new ClockModelTiming(timing_name);

		const PrototypeTiming* proto = world->findTiming(timing_name);
		if (!proto)
		{
			throw XmlImportException(
				"Timing source '" + timing_name + "' does not exist when processing receiver '" + name + "'");
		}
		//Initialize the new model from the prototype model
		timing->initializeModel(proto);
		//Set the receiver's timing source
		receiver->setTiming(timing);

		// Get the NoDirect flag, which causes direct signals to be ignored
		if (getAttributeBool(recvXml, "nodirect", "", false))
		{
			receiver->setFlag(Receiver::FLAG_NODIRECT);
			rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Ignoring direct signals for receiver '%s'\n",
			                 receiver->getName().c_str());
		}

		// Get the NoPropagationLoss flag, which causes propagation loss to be ignored
		// for example, when propagation loss is calculated with AREPS
		if (getAttributeBool(recvXml, "nopropagationloss", "", false))
		{
			receiver->setFlag(Receiver::FLAG_NOPROPLOSS);
			rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Ignoring propagation losses for receiver '%s'\n",
			                 receiver->getName().c_str());
		}

		//Add the receiver to the world
		world->add(receiver);

		return receiver;
	}

	/// Create a PulseTransmitter object and process XML entry
	Transmitter* processPulseTransmitter(const TiXmlHandle& transXml, std::string& name, const Platform* platform,
	                                     World* world)
	{
		auto* transmitter = new Transmitter(platform, string(name), true);
		//Get the name of the pulse
		const string pulse_name = getAttributeString(transXml, "pulse",
		                                             "Transmitter '" + name + "' does not specify a pulse");
		//Get the pulse from the table of pulses
		RadarSignal* wave = world->findSignal(pulse_name);
		if (!wave)
		{
			throw XmlImportException("Pulse with name '" + pulse_name + "' does not exist");
		}
		//Get the Pulse Repetition Frequency
		const RS_FLOAT prf = getChildRsFloat(transXml, "prf");
		//Attach the pulse to the transmitter
		transmitter->setWave(wave);
		transmitter->setPrf(prf);
		return transmitter;
	}

	/// Create a PulseTransmitter object and process XML entry
	Transmitter* processCwTransmitter(const TiXmlHandle& transXml, std::string& name, const Platform* platform,
	                                  World* world)
	{
		auto* transmitter = new Transmitter(platform, string(name), false);
		//Get the name of the pulse
		const string pulse_name = getAttributeString(transXml, "pulse",
		                                             "Transmitter '" + name + "' does not specify a pulse");
		//Get the pulse from the table of pulses
		RadarSignal* wave = world->findSignal(pulse_name);
		if (!wave)
		{
			throw XmlImportException("Pulse with name '" + pulse_name + "' does not exist");
		}
		//Attach the CW waveform to the transmitter
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
		rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Loading Transmitter: ");

		//Get the name of the transmitter
		string name = getAttributeString(transXml, "name", "Transmitter does not specify a name");

		//Get the transmitter type
		const string type = getAttributeString(transXml, "type", "Transmitter '" + name + "' does not specify type");
		Transmitter* transmitter;
		if (type == "pulsed")
		{
			transmitter = processPulseTransmitter(transXml, name, platform, world);
		}
		else if (type == "continuous")
		{
			transmitter = processCwTransmitter(transXml, name, platform, world);
		}
		else
		{
			throw XmlImportException("[ERROR] Invalid transmitter type specified in transmitter " + name);
		}

		//Get the name of the antenna
		const string ant_name = getAttributeString(transXml, "antenna",
		                                           "Transmitter '" + name + "' does not specify an antenna");
		const Antenna* antenna = world->findAntenna(ant_name);
		if (!antenna)
		{
			throw XmlImportException(
				"Antenna with name '" + ant_name + "' does not exist when processing Transmitter " + string(name));
		}
		//Set the transmitter's antenna
		transmitter->setAntenna(antenna);

		//Get the name of the timing source
		const string timing_name = getAttributeString(transXml, "timing",
		                                              "Transmitter '" + name + "' does not specify a timing source");

		auto* timing = new ClockModelTiming(name);

		const PrototypeTiming* proto = world->findTiming(timing_name);
		if (!proto)
		{
			throw XmlImportException(
				"Timing source '" + timing_name + "' does not exist when processing receiver " + name);
		}
		//Initialize the new model from the prototype model
		timing->initializeModel(proto);
		//Set the receiver's timing source
		transmitter->setTiming(timing);

		//Add the transmitter to the world
		world->add(transmitter);

		// If the system is monostatic, return the transmitter
		if (isMonostatic)
		{
			return transmitter;
		}
		return std::nullopt;
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
	void processWaypoint(const TiXmlHandle& handXml, Path* path)
	{
		try
		{
			const RS_FLOAT x = getChildRsFloat(handXml, "x");
			const RS_FLOAT y = getChildRsFloat(handXml, "y");
			const RS_FLOAT z = getChildRsFloat(handXml, "altitude");
			const RS_FLOAT t = getChildRsFloat(handXml, "time");
			Coord coord;
			coord.t = t;
			coord.pos = Vec3(x, y, z);
			path->addCoord(coord);
		}
		catch ([[maybe_unused]] XmlImportException& e)
		{
			rs_debug::printf(rs_debug::RS_VERBOSE,
			                 "[WARNING] Parse Error While Importing Waypoint. Discarding Waypoint.\n");
		}
	}

	/// Process the path's python attributes
	void processPythonPath(const TiXmlHandle& pathXml, Path* path)
	{
		//Initialize python, if it isn't done already
		rs_python::initPython();
		//Get the python path definition
		try
		{
			const TiXmlHandle tmp = pathXml.ChildElement("pythonpath", 0);
			//Get the module and function name attributes
			const std::string modname = getAttributeString(tmp, "module", "Attribute module missing");
			const std::string funcname = getAttributeString(tmp, "function", "Attribute function missing");
			//Load the Path module
			path->loadPythonPath(modname, funcname);
		}
		catch (XmlImportException& e)
		{
			rs_debug::printf(rs_debug::RS_VERBOSE, "%s", e.what());
		}
	}

	/// Process a MotionPath XML entry
	void processMotionPath(const TiXmlHandle& mpXml, const Platform* platform)
	{
		//Get a pointer to the platform's path
		Path* path = platform->getMotionPath();
		//Get the interpolation type
		try
		{
			if (const std::string rottype = getAttributeString(mpXml, "interpolation", ""); rottype == "linear")
			{
				path->setInterp(Path::RS_INTERP_LINEAR);
			}
			else if (rottype == "cubic")
			{
				path->setInterp(Path::RS_INTERP_CUBIC);
			}
			else if (rottype == "static")
			{
				path->setInterp(Path::RS_INTERP_STATIC);
			}
			else if (rottype == "python")
			{
				path->setInterp(Path::RS_INTERP_PYTHON);
				processPythonPath(mpXml, path);
			}
			else
			{
				rs_debug::printf(rs_debug::RS_VERBOSE,
				                 "[WARNING] Unsupported motion path interpolation type for platform '" + platform->
				                 getName() + "'. Defaulting to static.\n");
				path->setInterp(Path::RS_INTERP_STATIC);
			}
		}
		catch ([[maybe_unused]] XmlImportException& e)
		{
			rs_debug::printf(rs_debug::RS_VERBOSE,
			                 "[WARNING] Motion path interpolation type not specified for platform '" + platform->
			                 getName() + "'. Defaulting to static.\n");
			path->setInterp(Path::RS_INTERP_STATIC);
		}

		//Process all the PositionWaypoints
		TiXmlHandle tmp = mpXml.ChildElement("positionwaypoint", 0);
		for (int i = 1; tmp.Element() != nullptr; i++)
		{
			processWaypoint(tmp, path);
			tmp = mpXml.ChildElement("positionwaypoint", i);
		}
		//Finalise the path after all the waypoints have been loaded
		path->finalize();
	}

	/// Process a rotation path waypoint
	void processRotationWaypoint(const TiXmlHandle& handXml, RotationPath* path)
	{
		try
		{
			RotationCoord coord;
			coord.elevation = getChildRsFloat(handXml, "elevation");
			coord.azimuth = getChildRsFloat(handXml, "azimuth");
			coord.t = getChildRsFloat(handXml, "time");
			path->addCoord(coord);
		}
		catch ([[maybe_unused]] XmlImportException& e)
		{
			rs_debug::printf(rs_debug::RS_VERBOSE,
			                 "[WARNING] Parse Error While Importing Waypoint. Discarding Waypoint.\n");
		}
	}

	/// Process Waypoints for RotationPath
	void processRotationWaypoints(const TiXmlHandle& mpXml, RotationPath* path)
	{
		//Process all the RotationWaypoints
		TiXmlHandle tmp = mpXml.ChildElement("rotationwaypoint", 0);
		for (int i = 1; tmp.Element() != nullptr; i++)
		{
			processRotationWaypoint(tmp, path);
			tmp = mpXml.ChildElement("rotationwaypoint", i);
		}
		//Finalise the path after all the waypoints have been loaded
		path->finalize();
	}

	/// Process an entry for a fixed rotation
	void processRotationConstant(const TiXmlHandle& mpXml, const Platform* platform)
	{
		RotationPath* path = platform->getRotationPath();
		try
		{
			RotationCoord start, rate;
			start.azimuth = getChildRsFloat(mpXml, "startazimuth");
			start.elevation = getChildRsFloat(mpXml, "startelevation");
			rate.azimuth = getChildRsFloat(mpXml, "azimuthrate");
			rate.elevation = getChildRsFloat(mpXml, "elevationrate");
			path->setConstantRate(start, rate);
		}
		catch ([[maybe_unused]] XmlImportException& e)
		{
			rs_debug::printf(rs_debug::RS_VERBOSE, "[WARNING] Parse Error While Importing Constant Rotation.\n");
		}
	}

	/// Process a RotationPath XML entry
	void processRotationPath(const TiXmlHandle& mpXml, const Platform* platform)
	{
		rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Loading Rotation Path.\n");

		//Get a pointer to the rotation path
		RotationPath* path = platform->getRotationPath();

		//Get the interpolation type
		try
		{
			if (const std::string rottype = getAttributeString(mpXml, "interpolation", ""); rottype == "linear")
			{
				path->setInterp(RotationPath::RS_INTERP_LINEAR);
			}
			else if (rottype == "cubic")
			{
				path->setInterp(RotationPath::RS_INTERP_CUBIC);
			}
			else if (rottype == "static")
			{
				path->setInterp(RotationPath::RS_INTERP_STATIC);
			}
			else
			{
				rs_debug::printf(rs_debug::RS_VERBOSE,
				                 "[WARNING] Unsupported rotation path interpolation type for platform '" + platform->
				                 getName() + "'. Defaulting to static.\n");
				path->setInterp(RotationPath::RS_INTERP_STATIC);
			}
		}
		catch ([[maybe_unused]] XmlImportException& e)
		{
			rs_debug::printf(rs_debug::RS_VERBOSE,
			                 "[WARNING] Rotation path interpolation type not specified for platform '" + platform->
			                 getName() + "'. Defaulting to static.\n");
			path->setInterp(RotationPath::RS_INTERP_STATIC);
		}
		// Process the rotation waypoints
		processRotationWaypoints(mpXml, path);
	}

	/// Process a platform, recursively processing all the elements that are attached to it
	void processPlatform(const TiXmlHandle& platXml, World* world)
	{
		//Create the platform, using the name from the element
		std::string name = getAttributeString(platXml, "name", "[ERROR] Platform must specify a name");
		auto* platform = new Platform(string(name));
		//Add the platform to the world
		world->add(platform);

		//Process all the targets attached to the platform
		TiXmlHandle tmp = platXml.ChildElement("target", 0);
		for (int i = 1; tmp.Element() != nullptr; i++)
		{
			processTarget(tmp, platform, world);
			tmp = platXml.ChildElement("target", i);
		}

		//Process all the receivers attached to the platform
		tmp = platXml.ChildElement("receiver", 0);
		for (int i = 1; tmp.Element() != nullptr; i++)
		{
			processReceiver(tmp, platform, world);
			tmp = platXml.ChildElement("receiver", i);
		}

		//Process all the transmitters attached to the platform
		tmp = platXml.ChildElement("transmitter", 0);
		for (int i = 1; tmp.Element() != nullptr; i++)
		{
			processTransmitter(tmp, platform, world);
			tmp = platXml.ChildElement("transmitter", i);
		}

		//Process all the monostatics attached to the platform
		tmp = platXml.ChildElement("monostatic", 0);
		for (int i = 1; tmp.Element() != nullptr; i++)
		{
			processMonostatic(tmp, platform, world);
			tmp = platXml.ChildElement("monostatic", i);
		}

		//Process all the motion paths attached to the platform
		tmp = platXml.ChildElement("motionpath", 0);
		for (int i = 1; tmp.Element() != nullptr; i++)
		{
			processMotionPath(tmp, platform);
			tmp = platXml.ChildElement("motionpath", i);
		}

		//Process all the rotation paths attached to the platform
		tmp = platXml.ChildElement("rotationpath", 0);
		for (int i = 1; tmp.Element() != nullptr; i++)
		{
			processRotationPath(tmp, platform);
			tmp = platXml.ChildElement("rotationpath", i);
		}

		//Process all the rotation paths attached to the platform
		tmp = platXml.ChildElement("fixedrotation", 0);
		for (int i = 1; tmp.Element() != nullptr; i++)
		{
			processRotationConstant(tmp, platform);
			tmp = platXml.ChildElement("fixedrotation", i);
		}
	}


	/// Process a pulse entry of type rect
	void processAnyPulseFile(const TiXmlHandle& pulseXml, World* world, const std::string& name)
	{
		const string filename = getAttributeString(pulseXml, "filename", "Pulse must specify a filename");
		const RS_FLOAT carrier = getChildRsFloat(pulseXml, "carrier");
		const RS_FLOAT power = getChildRsFloat(pulseXml, "power");
		RadarSignal* wave = rs_pulse_factory::loadPulseFromFile(name, filename, power, carrier);
		world->add(wave);
	}

	/// Process a pulse entry
	void processPulse(const TiXmlHandle& pulseXml, World* world)
	{
		//Get the name of the pulse
		const string pulse_name = getAttributeString(pulseXml, "name", "Pulses must specify a name");
		//Get the type of the pulse
		const string pulse_type = getAttributeString(pulseXml, "type", "Pulses must specify a type");
		//Generate the pulse
		rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Generating Pulse %s of type '%s'\n", pulse_name.c_str(),
		                 pulse_type.c_str());
		if (pulse_type == "file")
		{
			processAnyPulseFile(pulseXml, world, pulse_name);
		}
		else
		{
			throw XmlImportException("Unrecognised type in pulse");
		}
	}

	Antenna* processPythonAntenna(const TiXmlHandle& antXml, const string& name)
	{
		//Initialize python, if it isn't done already
		rs_python::initPython();
		//Get the module and function name attributes
		const std::string modname = getAttributeString(antXml, "module", "Attribute module missing");
		const std::string funcname = getAttributeString(antXml, "function", "Attribute function missing");
		//Create the antenna
		return createPythonAntenna(name, modname, funcname);
	}


	Antenna* processXmlAntenna(const TiXmlHandle& antXml, const string& name)
	{
		//Get the module and function name attributes
		const std::string filename = getAttributeString(antXml, "filename",
		                                                "Antenna definition must specify a filename");
		//Create the antenna
		return createXmlAntenna(name, filename);
	}

	Antenna* processFileAntenna(const TiXmlHandle& antXml, const string& name)
	{
		//Get the module and function name attributes
		const std::string filename = getAttributeString(antXml, "filename",
		                                                "Antenna definition must specify a filename");
		//Create the antenna
		return createFileAntenna(name, filename);
	}

	Antenna* processSincAntenna(const TiXmlHandle& antXml, const string& name)
	{
		const RS_FLOAT alpha = getChildRsFloat(antXml, "alpha");
		const RS_FLOAT beta = getChildRsFloat(antXml, "beta");
		const RS_FLOAT gamma = getChildRsFloat(antXml, "gamma");
		return createSincAntenna(name, alpha, beta, gamma);
	}

	Antenna* processGaussianAntenna(const TiXmlHandle& antXml, const string& name)
	{
		const RS_FLOAT azscale = getChildRsFloat(antXml, "azscale");
		const RS_FLOAT elscale = getChildRsFloat(antXml, "elscale");
		return createGaussianAntenna(name, azscale, elscale);
	}

	Antenna* processParabolicAntenna(const TiXmlHandle& antXml, const string& name)
	{
		const RS_FLOAT diameter = getChildRsFloat(antXml, "diameter");
		return createParabolicAntenna(name, diameter);
	}

	void processAntenna(const TiXmlHandle& antXml, World* world)
	{
		//Get the name of the antenna
		const string ant_name = getAttributeString(antXml, "name", "Antennas must specify a name");
		//Get the type of the antenna
		const string ant_pattern = getAttributeString(antXml, "pattern", "Antennas must specify a pattern");
		Antenna* antenna;
		if (ant_pattern == "isotropic")
		{
			antenna = createIsotropicAntenna(ant_name);
		}
		else if (ant_pattern == "file")
		{
			antenna = processFileAntenna(antXml, ant_name);
		}
		else if (ant_pattern == "xml")
		{
			antenna = processXmlAntenna(antXml, ant_name);
		}
		else if (ant_pattern == "python")
		{
			antenna = processPythonAntenna(antXml, ant_name);
		}
		else if (ant_pattern == "sinc")
		{
			antenna = processSincAntenna(antXml, ant_name);
		}
		else if (ant_pattern == "gaussian")
		{
			antenna = processGaussianAntenna(antXml, ant_name);
		}
		else if (ant_pattern == "parabolic")
		{
			antenna = processParabolicAntenna(antXml, ant_name);
		}
		else
		{
			throw XmlImportException("Antenna specified unrecognised gain pattern '" + ant_pattern + "'");
		}
		//Notify the debug log
		rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Loading antenna '%s' of type '%s'\n", ant_name.c_str(),
		                 ant_pattern.c_str());
		//Load the efficiency factor
		try
		{
			const RS_FLOAT factor = getChildRsFloat(antXml, "efficiency");
			antenna->setEfficiencyFactor(factor);
		}
		catch ([[maybe_unused]] XmlImportException& xe)
		{
			rs_debug::printf(rs_debug::RS_VERBOSE,
			                 "[VERBOSE] Antenna '%s' does not specify efficiency, assuming unity.\n", ant_name.c_str());
		}
		//Add it to the world
		world->add(antenna);
	}

	/// Process a multipath surface and add it to the world
	void processMultipath(const TiXmlHandle& mpXml, World* world)
	{
		//Get the reflecting factor
		const RS_FLOAT factor = getChildRsFloat(mpXml, "factor");
		const RS_FLOAT nx = getChildRsFloat(mpXml, "nx");
		const RS_FLOAT ny = getChildRsFloat(mpXml, "ny");
		const RS_FLOAT nz = getChildRsFloat(mpXml, "nz");
		const RS_FLOAT d = getChildRsFloat(mpXml, "d");
		//Create the multipath object
		auto* mps = new MultipathSurface(nx, ny, nz, d, factor);
		//Add it to the world
		world->addMultipathSurface(mps);
	}

	/// Process a timing source and add it to the world
	void processTiming(const TiXmlHandle& antXml, World* world)
	{
		//Get the name of the antenna
		const string name = getAttributeString(antXml, "name", "Timing sources must specify a name");
		auto* timing = new PrototypeTiming(name);
		//Process all the clock entries
		TiXmlHandle plat = antXml.ChildElement("noise_entry", 0);
		for (int i = 1; plat.Element() != nullptr; i++)
		{
			const RS_FLOAT alpha = getChildRsFloat(plat, "alpha");
			const RS_FLOAT weight = getChildRsFloat(plat, "weight");
			timing->addAlpha(alpha, weight);
			plat = antXml.ChildElement("noise_entry", i);
		}
		// Process the frequency offset
		try
		{
			const RS_FLOAT offset = getChildRsFloat(antXml, "freq_offset");
			timing->addFreqOffset(offset);
		}
		catch ([[maybe_unused]] XmlImportException& xe)
		{
		}
		try
		{
			const RS_FLOAT stdev = getChildRsFloat(antXml, "random_freq_offset");
			timing->addRandomFreqOffset(stdev);
		}
		catch ([[maybe_unused]] XmlImportException& xe)
		{
		}
		// Process the phase offset
		try
		{
			const RS_FLOAT offset = getChildRsFloat(antXml, "phase_offset");
			timing->addPhaseOffset(offset);
		}
		catch ([[maybe_unused]] XmlImportException& xe)
		{
		}
		try
		{
			const RS_FLOAT stdev = getChildRsFloat(antXml, "random_phase_offset");
			timing->addRandomPhaseOffset(stdev);
		}
		catch ([[maybe_unused]] XmlImportException& xe)
		{
		}
		// Process the frequency
		try
		{
			const RS_FLOAT freq = getChildRsFloat(antXml, "frequency");
			timing->setFrequency(freq);
		}
		catch ([[maybe_unused]] XmlImportException& xe)
		{
			//If there is no frequency, we default to the system sample frequency
			timing->setFrequency(RsParameters::rate());
			rs_debug::printf(rs_debug::RS_VERBOSE,
			                 "[VERBOSE] Clock section '%s' does not specify frequency. Assuming %g.\n", name.c_str(),
			                 RsParameters::rate());
		}
		//Process the synconpulse tag
		if (getAttributeBool(antXml, "synconpulse", "", true))
		{
			timing->setSyncOnPulse();
		}
		//Notify the debug log
		rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Loading timing source '%s'\n", name.c_str());

		//Add it to the world
		world->add(timing);
	}

	/// Process the <parameters> element
	void processParameters(const TiXmlHandle& root)
	{
		//Get the simulation start and end times
		RsParameters::modifyParms()->setTime(getChildRsFloat(root, "starttime"), getChildRsFloat(root, "endtime"));
		//Get the propagation speed in air
		try
		{
			const RS_FLOAT c = getChildRsFloat(root, "c");
			RsParameters::modifyParms()->setC(c);
		}
		catch ([[maybe_unused]] XmlImportException& xe)
		{
			rs_debug::printf(rs_debug::RS_VERBOSE, "[VERBOSE] Using default value of c: %f(m/s)\n", RsParameters::c());
		}
		//Get the export sampling rate
		try
		{
			const RS_FLOAT rate = getChildRsFloat(root, "rate");
			RsParameters::modifyParms()->setRate(rate);
		}
		catch ([[maybe_unused]] XmlImportException& xe)
		{
			rs_debug::printf(rs_debug::RS_VERBOSE, "[VERBOSE] Using default sampling rate.\n");
		}
		//Get the cw Interpolation rate
		try
		{
			const RS_FLOAT rate = getChildRsFloat(root, "interprate");
			RsParameters::modifyParms()->setCwSampleRate(rate);
		}
		catch ([[maybe_unused]] XmlImportException& xe)
		{
			rs_debug::printf(rs_debug::RS_VERBOSE,
			                 "[VERBOSE] Using default value of CW position interpolation rate: %g\n",
			                 RsParameters::cwSampleRate());
		}
		//Get the random seed
		try
		{
			const RS_FLOAT seed = getChildRsFloat(root, "randomseed");
			RsParameters::modifyParms()->setRandomSeed(static_cast<unsigned int>(std::fabs(seed)));
		}
		catch ([[maybe_unused]] XmlImportException& xe)
		{
			rs_debug::printf(rs_debug::RS_VERBOSE, "[VERBOSE] Using random seed from clock(): %d\n",
			                 RsParameters::randomSeed());
		}
		//Get the number of ADC bits to simulate
		try
		{
			const RS_FLOAT adc_bits = getChildRsFloat(root, "adc_bits");
			RsParameters::modifyParms()->setAdcBits(static_cast<unsigned int>(std::floor(adc_bits)));
			rs_debug::printf(rs_debug::RS_VERBOSE, "[VERBOSE] Quantizing results to %d bits\n",
			                 RsParameters::adcBits());
		}
		catch ([[maybe_unused]] XmlImportException& xe)
		{
			rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VERBOSE] Using full precision simulation.\n");
		}
		// Get the oversampling ratio
		try
		{
			const RS_FLOAT ratio = getChildRsFloat(root, "oversample");
			RsParameters::modifyParms()->setOversampleRatio(static_cast<unsigned int>(std::floor(ratio)));
		}
		catch ([[maybe_unused]] XmlImportException& xe)
		{
			rs_debug::printf(rs_debug::RS_VERY_VERBOSE,
			                 "[VV] Oversampling not in use. Ensure than pulses are correctly sampled.\n");
		}
		//Process the "export" tag
		if (const TiXmlHandle exporttag = root.ChildElement("export", 0); exporttag.Element())
		{
			const bool export_xml = getAttributeBool(exporttag, "xml", "", RsParameters::exportXml());
			const bool export_csv = getAttributeBool(exporttag, "csv", "", RsParameters::exportCsv());
			const bool export_binary = getAttributeBool(exporttag, "binary", "", RsParameters::exportBinary());
			RsParameters::modifyParms()->setExporters(export_xml, export_csv, export_binary);
		}
	}

	/// Process the XML tree, starting at the root
	void processDocument(const TiXmlHandle& root, World* world, bool included);

	/// Process the inclusion of a file
	void processInclude(const TiXmlHandle& plat, World* world)
	{
		const char* name = getNodeText(plat);
		TiXmlDocument doc(name);
		if (!doc.LoadFile())
		{
			throw std::runtime_error("Cannot open included file: " + std::string(name));
		}
		//Process the XML document
		const TiXmlHandle root(doc.RootElement());
		processDocument(root, world, true);
	}

	/// Process the XML tree, starting at the root
	void processDocument(const TiXmlHandle& root, World* world, const bool included)
	{
		if (!included)
		{
			//Process the parameters
			const TiXmlHandle parameters = root.ChildElement("parameters", 0);
			processParameters(parameters);
		}
		//Process all the pulses
		TiXmlHandle plat = root.ChildElement("pulse", 0);
		for (int i = 1; plat.Element() != nullptr; i++)
		{
			processPulse(plat, world);
			plat = root.ChildElement("pulse", i);
		}
		//Process all the antennas
		plat = root.ChildElement("antenna", 0);
		for (int i = 1; plat.Element() != nullptr; i++)
		{
			processAntenna(plat, world);
			plat = root.ChildElement("antenna", i);
		}
		//Process all the timing sources
		plat = root.ChildElement("timing", 0);
		for (int i = 1; plat.Element() != nullptr; i++)
		{
			processTiming(plat, world);
			plat = root.ChildElement("timing", i);
		}
		//Process all the multipath surfaces
		plat = root.ChildElement("multipath", 0);
		for (int i = 1; plat.Element() != nullptr; i++)
		{
			processMultipath(plat, world);
			plat = root.ChildElement("multipath", i);
		}
		//Process all the platforms
		plat = root.ChildElement("platform", 0);
		for (int i = 1; plat.Element() != nullptr; i++)
		{
			processPlatform(plat, world); //Recursively process the platform
			plat = root.ChildElement("platform", i);
		}
		//Process all the includes
		plat = root.ChildElement("include", 0);
		for (int i = 1; plat.Element() != nullptr; i++)
		{
			processInclude(plat, world); //Recursively process the platform
			plat = root.ChildElement("include", i);
		}
		//Process all the incblocks
		plat = root.ChildElement("incblock", 0);
		for (int i = 1; plat.Element() != nullptr; i++)
		{
			processDocument(plat, world, true); //Recursively process the platform
			plat = root.ChildElement("incblock", i);
		}
	}
} //Anonymous Namespace

/// Load a XML file into the world with the given filename
void xml::loadXmlFile(const string& filename, World* world)
{
	TiXmlDocument doc(filename.c_str());
	if (!doc.LoadFile())
	{
		throw std::runtime_error("Cannot open script file");
	}
	//Process the XML document
	const TiXmlHandle root(doc.RootElement());
	processDocument(root, world, false);
	//Create multipath duals of all objects, if a surface was added
	world->processMultipath();
}
