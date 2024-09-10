//xmlimport.cpp
//Import a simulator world and simulation parameters from an XML file
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started 26 April 2006

//TODO: Rewrite this code to be less ugly

#define TIXML_USE_STL //Tell tinyxml to use the STL instead of it's own string class

#include "xmlimport.h"

#include <cmath>
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
	XmlImportException(const std::string& error):
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
const char* GetChildText(const TiXmlHandle& parent, const char* childname)
{
	const TiXmlHandle tmp = parent.FirstChildElement(childname);
	if (tmp.Element() == 0)
	{
		return 0; //The element does not exist
	}
	//Return the text
	return tmp.Element()->GetText();
}

/// Gets child text as a rsFloat. See GetChildText for usage description
rsFloat GetChildRsFloat(const TiXmlHandle& parent, const char* childname)
{
	const char* data = GetChildText(parent, childname);
	//If there is no data, flag failure and return
	if (!data)
	{
		throw XmlImportException("No data in child element " + string(childname) + " during GetChildRsFloat.");
	}
	//Parse the first rsFloat from the data
	rsFloat result;
	std::istringstream iss(data);
	iss >> result;
	return result;
}

/// Get the text contents of a node
const char* GetNodeText(const TiXmlHandle& parent)
{
	//Return the text
	return parent.Element()->GetText();
}

/// Gets the text content of a node as an rsFloat.
//For XML like this:
//<rcs>10</rcs>
rsFloat GetNodeFloat(const TiXmlHandle& node)
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
	rsFloat result;
	std::istringstream iss(data);
	iss >> result;
	return result;
}

/// Return the string associated with an attribute or throw an exception on failure
std::string GetAttributeString(const TiXmlHandle& handle, const std::string& name, const std::string& error, const bool optional = false)
{
	if (const std::string* text = handle.Element()->Attribute(name))
	{
		return *text;
	}
	else
	{
		if (!optional)
		{
			throw XmlImportException(error);
		}
		else
		{
			return string("");
		}
	}
}

/// Return the bool associated with an attribute
bool GetAttributeBool(const TiXmlHandle& handle, const std::string& name, const std::string& error, const bool def, const bool optional = true)
{
	const string str = GetAttributeString(handle, name, error, optional);
	if (str == "")
	{
		return def;
	}
	return ((str == "true") || (str == "yes"));
}


namespace
{
	///Process a Gamma target model entry
	RCSModel* ProcessGammaModel(const TiXmlHandle& modelXML)
	{
		const rsFloat k = GetChildRsFloat(modelXML, "k");
		return new RCSChiSquare(k);
	}

	/// Process a target XML entry
	void ProcessTarget(const TiXmlHandle& targXML, const Platform* platform, World* world)
	{
		Target* target;
		const string name = GetAttributeString(targXML, "name", "Target does not specify a name");
		//Get the RCS
		const TiXmlHandle rcsXML = targXML.ChildElement("rcs", 0);
		if (!rcsXML.Element())
		{
			throw XmlImportException("Target " + name + " does not specify RCS.");
		}
		const string rcs_type = GetAttributeString(rcsXML, "type",
		                                           "RCS attached to target '" + name + "' does not specify type.");
		// Handle the target type (isotropic, file imported, etc.)
		if (rcs_type == "isotropic")
		{
			const TiXmlHandle rcsValueXML = rcsXML.ChildElement("value", 0);
			if (!rcsValueXML.Element())
			{
				throw XmlImportException("Target " + name + " does not specify value of isotropic RCS.");
			}
			const rsFloat value = GetNodeFloat(rcsValueXML);
			target = CreateIsoTarget(platform, name, value);
		}
		else if (rcs_type == "file")
		{
			const string filename = GetAttributeString(rcsXML, "filename",
			                                     "RCS attached to target '" + name + "' does not specify filename.");
			target = CreateFileTarget(platform, name, filename);
		}
		else
		{
			throw XmlImportException("RCS type " + rcs_type + " not currently supported.");
		}
		//Handle the target statistical model
		if (const TiXmlHandle modelXML = targXML.ChildElement("model", 0); modelXML.Element())
		{
			//Get the mode type
			const string model_type = GetAttributeString(modelXML, "type",
			                                             "Model attached to target '" + name + "' does not specify type.");
			if (model_type == "constant")
			{
				RCSConst* model = new RCSConst();
				target->SetFluctuationModel(model);
			}
			else if ((model_type == "chisquare") || (model_type == "gamma"))
			{
				RCSModel* model = ProcessGammaModel(modelXML);
				target->SetFluctuationModel(model);
			}
			else
			{
				throw XmlImportException("Target fluctuation model type '" + model_type + "' not recognised.");
			}
		}
		//Add the target to the world
		world->Add(target);
	}

	/// Process a receiver XML entry
	Receiver* ProcessReceiver(const TiXmlHandle& recvXML, const Platform* platform, World* world)
	{
		rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Loading Receiver: ");

		//Get the name of the receiver
		string name = GetAttributeString(recvXML, "name", "Receiver does not specify a name");
		Receiver* receiver = new Receiver(platform, name);

		//Get the name of the antenna
		const string ant_name = GetAttributeString(recvXML, "antenna",
		                                     "Receiver '" + string(name) + "' does not specify an antenna");

		rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "'%s' ", receiver->GetName().c_str());

		const Antenna* antenna = world->FindAntenna(ant_name);
		if (!antenna)
		{
			throw XmlImportException(
				"Antenna with name '" + ant_name + "' does not exist when processing Receiver " + string(name));
		}
		//Set the receiver's antenna
		receiver->SetAntenna(antenna);

		//Process the noise temperature tag
		try
		{
			const rsFloat temperature = GetChildRsFloat(recvXML, "noise_temp");
			receiver->SetNoiseTemperature(temperature);
		}
		catch (XmlImportException& e)
		{
		}

		//Process the PRF tag
		const rsFloat prf = GetChildRsFloat(recvXML, "prf");
		const rsFloat skip = GetChildRsFloat(recvXML, "window_skip");
		const rsFloat length = GetChildRsFloat(recvXML, "window_length");
		receiver->SetWindowProperties(length, prf, skip);

		//Get the name of the timing source
		const string timing_name = GetAttributeString(recvXML, "timing",
		                                              "Receiver '" + name + "' does not specify a timing source");
		ClockModelTiming* timing = new ClockModelTiming(timing_name);

		const PrototypeTiming* proto = world->FindTiming(timing_name);
		if (!proto)
		{
			throw XmlImportException(
				"Timing source '" + timing_name + "' does not exist when processing receiver '" + name + "'");
		}
		//Initialize the new model from the prototype model
		timing->InitializeModel(proto);
		//Set the receiver's timing source
		receiver->SetTiming(timing);

		// Get the NoDirect flag, which causes direct signals to be ignored
		if (bool nodirect = GetAttributeBool(recvXML, "nodirect", "", false))
		{
			receiver->SetFlag(rs::Receiver::FLAG_NODIRECT);
			rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Ignoring direct signals for receiver '%s'\n",
			                receiver->GetName().c_str());
		}

		// Get the NoPropagationLoss flag, which causes propagation loss to be ignored
		// for example, when propagation loss is calculated with AREPS
		if (bool noproploss = GetAttributeBool(recvXML, "nopropagationloss", "", false))
		{
			receiver->SetFlag(rs::Receiver::FLAG_NOPROPLOSS);
			rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Ignoring propagation losses for receiver '%s'\n",
			                receiver->GetName().c_str());
		}

		//Add the receiver to the world
		world->Add(receiver);

		return receiver;
	}

	/// Create a PulseTransmitter object and process XML entry
	Transmitter* ProcessPulseTransmitter(const TiXmlHandle& transXML, std::string& name, const Platform* platform, World* world)
	{
		Transmitter* transmitter = new Transmitter(platform, string(name), true);
		//Get the name of the pulse
		const string pulse_name = GetAttributeString(transXML, "pulse",
		                                       "Transmitter '" + name + "' does not specify a pulse");
		//Get the pulse from the table of pulses
		RadarSignal* wave = world->FindSignal(pulse_name);
		if (!wave)
		{
			throw XmlImportException("Pulse with name '" + pulse_name + "' does not exist");
		}
		//Get the Pulse Repetition Frequency
		const rsFloat prf = GetChildRsFloat(transXML, "prf");
		//Attach the pulse to the transmitter
		transmitter->SetWave(wave);
		transmitter->SetPRF(prf);
		return transmitter;
	}

	/// Create a PulseTransmitter object and process XML entry
	Transmitter* ProcessCWTransmitter(const TiXmlHandle& transXML, std::string& name, const Platform* platform, World* world)
	{
		Transmitter* transmitter = new Transmitter(platform, string(name), false);
		//Get the name of the pulse
		const string pulse_name = GetAttributeString(transXML, "pulse",
		                                       "Transmitter '" + name + "' does not specify a pulse");
		//Get the pulse from the table of pulses
		RadarSignal* wave = world->FindSignal(pulse_name);
		if (!wave)
		{
			throw XmlImportException("Pulse with name '" + pulse_name + "' does not exist");
		}
		//Attach the CW waveform to the transmitter
		transmitter->SetWave(wave);
		return transmitter;
	}

	/// Process a transmitter XML entry
	Transmitter* ProcessTransmitter(const TiXmlHandle& transXML, const Platform* platform, World* world)
	{
		rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Loading Transmitter: ");

		//Get the name of the transmitter
		string name = GetAttributeString(transXML, "name", "Transmitter does not specify a name");


		//Get the transmitter type
		const string type = GetAttributeString(transXML, "type", "Transmitter '" + name + "' does not specify type");
		Transmitter* transmitter;
		if (type == "pulsed")
		{
			transmitter = ProcessPulseTransmitter(transXML, name, platform, world);
		}
		else if (type == "continuous")
		{
			transmitter = ProcessCWTransmitter(transXML, name, platform, world);
		}
		else
		{
			throw XmlImportException("[ERROR] Invalid transmitter type specified in transmitter " + name);
		}

		//Get the name of the antenna
		const string ant_name = GetAttributeString(transXML, "antenna",
		                                     "Transmitter '" + name + "' does not specify an antenna");
		const Antenna* antenna = world->FindAntenna(ant_name);
		if (!antenna)
		{
			throw XmlImportException(
				"Antenna with name '" + ant_name + "' does not exist when processing Transmitter " + string(name));
		}
		//Set the transmitter's antenna
		transmitter->SetAntenna(antenna);

		//Get the name of the timing source
		const string timing_name = GetAttributeString(transXML, "timing",
		                                        "Transmitter '" + name + "' does not specify a timing source");

		ClockModelTiming* timing = new ClockModelTiming(name);

		const PrototypeTiming* proto = world->FindTiming(timing_name);
		if (!proto)
		{
			throw XmlImportException(
				"Timing source '" + timing_name + "' does not exist when processing receiver " + name);
		}
		//Initialize the new model from the prototype model
		timing->InitializeModel(proto);
		//Set the receiver's timing source
		transmitter->SetTiming(timing);

		//Add the transmitter to the world
		world->Add(transmitter);

		return transmitter;
	}

	/// Process a monostatic (Receiver and Transmitter sharing an antenna)
	void ProcessMonostatic(const TiXmlHandle& transXML, const Platform* platform, World* world)
	{
		Transmitter* trans = ProcessTransmitter(transXML, platform, world);
		Receiver* recv = ProcessReceiver(transXML, platform, world);
		trans->MakeMonostatic(recv);
		recv->MakeMonostatic(trans);
	}

	/// Process a motion path waypoint
	void ProcessWaypoint(const TiXmlHandle& handXML, Path* path)
	{
		try
		{
			const rsFloat x = GetChildRsFloat(handXML, "x");
			const rsFloat y = GetChildRsFloat(handXML, "y");
			const rsFloat z = GetChildRsFloat(handXML, "altitude");
			const rsFloat t = GetChildRsFloat(handXML, "time");
			Coord coord;
			coord.t = t;
			coord.pos = Vec3(x, y, z);
			path->AddCoord(coord);
		}
		catch (XmlImportException& e)
		{
			rsDebug::printf(rsDebug::RS_VERBOSE,
			                "[WARNING] Parse Error While Importing Waypoint. Discarding Waypoint.\n");
		}
	}

	/// Process the path's python attributes
	void ProcessPythonPath(const TiXmlHandle& pathXML, Path* path)
	{
		//Initialize python, if it isn't done already
		rsPython::InitPython();
		//Get the python path definition
		try
		{
			const TiXmlHandle tmp = pathXML.ChildElement("pythonpath", 0);
			//Get the module and function name attributes
			const std::string modname = GetAttributeString(tmp, "module", "Attribute module missing");
			const std::string funcname = GetAttributeString(tmp, "function", "Attribute function missing");
			//Load the Path module
			path->LoadPythonPath(modname, funcname);
		}
		catch (XmlImportException& e)
		{
			rsDebug::printf(rsDebug::RS_VERBOSE, "%s", e.what());
		}
	}

	/// Process a MotionPath XML entry
	void ProcessMotionPath(const TiXmlHandle& mpXML, Platform* platform)
	{
		//Get a pointer to the platform's path
		Path* path = platform->GetMotionPath();
		//Get the interpolation type
		try
		{
			if (const std::string rottype = GetAttributeString(mpXML, "interpolation", ""); rottype == "linear")
			{
				path->SetInterp(Path::RS_INTERP_LINEAR);
			}
			else if (rottype == "cubic")
			{
				path->SetInterp(Path::RS_INTERP_CUBIC);
			}
			else if (rottype == "static")
			{
				path->SetInterp(Path::RS_INTERP_STATIC);
			}
			else if (rottype == "python")
			{
				path->SetInterp(Path::RS_INTERP_PYTHON);
				ProcessPythonPath(mpXML, path);
			}
			else
			{
				rsDebug::printf(rsDebug::RS_VERBOSE,
				                "[WARNING] Unsupported motion path interpolation type for platform '" + platform->
				                GetName() + "'. Defaulting to static.\n");
				path->SetInterp(Path::RS_INTERP_STATIC);
			}
		}
		catch (XmlImportException& e)
		{
			rsDebug::printf(rsDebug::RS_VERBOSE,
			                "[WARNING] Motion path interpolation type not specified for platform '" + platform->
			                GetName() + "'. Defaulting to static.\n");
			path->SetInterp(Path::RS_INTERP_STATIC);
		}

		//Process all the PositionWaypoints
		TiXmlHandle tmp = mpXML.ChildElement("positionwaypoint", 0);
		for (int i = 1; tmp.Element() != 0; i++)
		{
			ProcessWaypoint(tmp, path);
			tmp = mpXML.ChildElement("positionwaypoint", i);
		}
		//Finalise the path after all the waypoints have been loaded
		path->Finalize();
	}

	/// Process a rotation path waypoint
	void ProcessRotationWaypoint(const TiXmlHandle& handXML, RotationPath* path)
	{
		try
		{
			RotationCoord coord;
			coord.elevation = GetChildRsFloat(handXML, "elevation");
			coord.azimuth = GetChildRsFloat(handXML, "azimuth");
			coord.t = GetChildRsFloat(handXML, "time");
			path->AddCoord(coord);
		}
		catch (XmlImportException& e)
		{
			rsDebug::printf(rsDebug::RS_VERBOSE,
			                "[WARNING] Parse Error While Importing Waypoint. Discarding Waypoint.\n");
		}
	}

	/// Process Waypoints for RotationPath
	void ProcessRotationWaypoints(const TiXmlHandle& mpXML, RotationPath* path)
	{
		//Process all the RotationWaypoints
		TiXmlHandle tmp = mpXML.ChildElement("rotationwaypoint", 0);
		for (int i = 1; tmp.Element() != 0; i++)
		{
			ProcessRotationWaypoint(tmp, path);
			tmp = mpXML.ChildElement("rotationwaypoint", i);
		}
		//Finalise the path after all the waypoints have been loaded
		path->Finalize();
	}

	/// Process an entry for a fixed rotation
	void ProcessRotationConstant(const TiXmlHandle& mpXML, Platform* platform)
	{
		RotationPath* path = platform->GetRotationPath();
		try
		{
			RotationCoord start, rate;
			start.azimuth = GetChildRsFloat(mpXML, "startazimuth");
			start.elevation = GetChildRsFloat(mpXML, "startelevation");
			rate.azimuth = GetChildRsFloat(mpXML, "azimuthrate");
			rate.elevation = GetChildRsFloat(mpXML, "elevationrate");
			path->SetConstantRate(start, rate);
		}
		catch (XmlImportException& e)
		{
			rsDebug::printf(rsDebug::RS_VERBOSE, "[WARNING] Parse Error While Importing Constant Rotation.\n");
		}
	}

	/// Process a RotationPath XML entry
	void ProcessRotationPath(const TiXmlHandle& mpXML, Platform* platform)
	{
		rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Loading Rotation Path.\n");

		//Get a pointer to the rotation path
		RotationPath* path = platform->GetRotationPath();

		//Get the interpolation type
		try
		{
			if (const std::string rottype = GetAttributeString(mpXML, "interpolation", ""); rottype == "linear")
			{
				path->SetInterp(RotationPath::RS_INTERP_LINEAR);
			}
			else if (rottype == "cubic")
			{
				path->SetInterp(RotationPath::RS_INTERP_CUBIC);
			}
			else if (rottype == "static")
			{
				path->SetInterp(RotationPath::RS_INTERP_STATIC);
			}
			else
			{
				rsDebug::printf(rsDebug::RS_VERBOSE,
				                "[WARNING] Unsupported rotation path interpolation type for platform '" + platform->
				                GetName() + "'. Defaulting to static.\n");
				path->SetInterp(RotationPath::RS_INTERP_STATIC);
			}
		}
		catch (XmlImportException& e)
		{
			rsDebug::printf(rsDebug::RS_VERBOSE,
			                "[WARNING] Rotation path interpolation type not specified for platform '" + platform->
			                GetName() + "'. Defaulting to static.\n");
			path->SetInterp(RotationPath::RS_INTERP_STATIC);
		}
		// Process the rotation waypoints
		ProcessRotationWaypoints(mpXML, path);
	}

	/// Process a platform, recursively processing all the elements that are attached to it
	void ProcessPlatform(const TiXmlHandle& platXML, World* world)
	{
		//Create the platform, using the name from the element
		std::string name = GetAttributeString(platXML, "name", "[ERROR] Platform must specify a name");
		Platform* platform = new Platform(string(name));
		//Add the platform to the world
		world->Add(platform);

		//Process all the targets attached to the platform
		TiXmlHandle tmp = platXML.ChildElement("target", 0);
		for (int i = 1; tmp.Element() != 0; i++)
		{
			ProcessTarget(tmp, platform, world);
			tmp = platXML.ChildElement("target", i);
		}

		//Process all the receivers attached to the platform
		tmp = platXML.ChildElement("receiver", 0);
		for (int i = 1; tmp.Element() != 0; i++)
		{
			ProcessReceiver(tmp, platform, world);
			tmp = platXML.ChildElement("receiver", i);
		}

		//Process all the transmitters attached to the platform
		tmp = platXML.ChildElement("transmitter", 0);
		for (int i = 1; tmp.Element() != 0; i++)
		{
			ProcessTransmitter(tmp, platform, world);
			tmp = platXML.ChildElement("transmitter", i);
		}

		//Process all the monostatics attached to the platform
		tmp = platXML.ChildElement("monostatic", 0);
		for (int i = 1; tmp.Element() != 0; i++)
		{
			ProcessMonostatic(tmp, platform, world);
			tmp = platXML.ChildElement("monostatic", i);
		}

		//Process all the motion paths attached to the platform
		tmp = platXML.ChildElement("motionpath", 0);
		for (int i = 1; tmp.Element() != 0; i++)
		{
			ProcessMotionPath(tmp, platform);
			tmp = platXML.ChildElement("motionpath", i);
		}

		//Process all the rotation paths attached to the platform
		tmp = platXML.ChildElement("rotationpath", 0);
		for (int i = 1; tmp.Element() != 0; i++)
		{
			ProcessRotationPath(tmp, platform);
			tmp = platXML.ChildElement("rotationpath", i);
		}

		//Process all the rotation paths attached to the platform
		tmp = platXML.ChildElement("fixedrotation", 0);
		for (int i = 1; tmp.Element() != 0; i++)
		{
			ProcessRotationConstant(tmp, platform);
			tmp = platXML.ChildElement("fixedrotation", i);
		}
	}


	/// Process a pulse entry of type rect
	void ProcessAnyPulseFile(const TiXmlHandle& pulseXML, World* world, const std::string& name)
	{
		const string filename = GetAttributeString(pulseXML, "filename", "Pulse must specify a filename");
		const rsFloat carrier = GetChildRsFloat(pulseXML, "carrier");
		const rsFloat power = GetChildRsFloat(pulseXML, "power");
		RadarSignal* wave = rsPulseFactory::LoadPulseFromFile(name, filename, power, carrier);
		world->Add(wave);
	}

	/// Process a pulse entry
	void ProcessPulse(const TiXmlHandle& pulseXML, World* world)
	{
		//Get the name of the pulse
		const string pulse_name = GetAttributeString(pulseXML, "name", "Pulses must specify a name");
		//Get the type of the pulse
		const string pulse_type = GetAttributeString(pulseXML, "type", "Pulses must specify a type");
		//Generate the pulse
		rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Generating Pulse %s of type '%s'\n", pulse_name.c_str(),
		                pulse_type.c_str());
		if (pulse_type == "file")
		{
			ProcessAnyPulseFile(pulseXML, world, pulse_name);
		}
		else
		{
			throw XmlImportException("Unrecognised type in pulse");
		}
	}

	Antenna* ProcessPythonAntenna(const TiXmlHandle& antXML, const string& name)
	{
		//Initialize python, if it isn't done already
		rsPython::InitPython();
		//Get the module and function name attributes
		const std::string modname = GetAttributeString(antXML, "module", "Attribute module missing");
		const std::string funcname = GetAttributeString(antXML, "function", "Attribute function missing");
		//Create the antenna
		return rs::CreatePythonAntenna(name, modname, funcname);
	}


	Antenna* ProcessXMLAntenna(const TiXmlHandle& antXML, const string& name)
	{
		//Get the module and function name attributes
		const std::string filename = GetAttributeString(antXML, "filename", "Antenna definition must specify a filename");
		//Create the antenna
		return rs::CreateXMLAntenna(name, filename);
	}

	Antenna* ProcessFileAntenna(const TiXmlHandle& antXML, const string& name)
	{
		//Get the module and function name attributes
		const std::string filename = GetAttributeString(antXML, "filename", "Antenna definition must specify a filename");
		//Create the antenna
		return rs::CreateFileAntenna(name, filename);
	}

	Antenna* ProcessSincAntenna(const TiXmlHandle& antXML, const string& name)
	{
		const rsFloat alpha = GetChildRsFloat(antXML, "alpha");
		const rsFloat beta = GetChildRsFloat(antXML, "beta");
		const rsFloat gamma = GetChildRsFloat(antXML, "gamma");
		return rs::CreateSincAntenna(name, alpha, beta, gamma);
	}

	Antenna* ProcessGaussianAntenna(const TiXmlHandle& antXML, const string& name)
	{
		const rsFloat azscale = GetChildRsFloat(antXML, "azscale");
		const rsFloat elscale = GetChildRsFloat(antXML, "elscale");
		return rs::CreateGaussianAntenna(name, azscale, elscale);
	}

	Antenna* ProcessParabolicAntenna(const TiXmlHandle& antXML, const string& name)
	{
		const rsFloat diameter = GetChildRsFloat(antXML, "diameter");
		return rs::CreateParabolicAntenna(name, diameter);
	}

	void ProcessAntenna(const TiXmlHandle& antXML, World* world)
	{
		//Get the name of the antenna
		const string ant_name = GetAttributeString(antXML, "name", "Antennas must specify a name");
		//Get the type of the antenna
		const string ant_pattern = GetAttributeString(antXML, "pattern", "Antennas must specify a pattern");
		Antenna* antenna;
		if (ant_pattern == "isotropic")
		{
			antenna = CreateIsotropicAntenna(ant_name);
		}
		else if (ant_pattern == "file")
		{
			antenna = ProcessFileAntenna(antXML, ant_name);
		}
		else if (ant_pattern == "xml")
		{
			antenna = ProcessXMLAntenna(antXML, ant_name);
		}
		else if (ant_pattern == "python")
		{
			antenna = ProcessPythonAntenna(antXML, ant_name);
		}
		else if (ant_pattern == "sinc")
		{
			antenna = ProcessSincAntenna(antXML, ant_name);
		}
		else if (ant_pattern == "gaussian")
		{
			antenna = ProcessGaussianAntenna(antXML, ant_name);
		}
		else if (ant_pattern == "parabolic")
		{
			antenna = ProcessParabolicAntenna(antXML, ant_name);
		}
		else
		{
			throw XmlImportException("Antenna specified unrecognised gain pattern '" + ant_pattern + "'");
		}
		//Notify the debug log
		rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Loading antenna '%s' of type '%s'\n", ant_name.c_str(),
		                ant_pattern.c_str());
		//Load the efficiency factor
		try
		{
			const rsFloat factor = GetChildRsFloat(antXML, "efficiency");
			antenna->SetEfficiencyFactor(factor);
		}
		catch (XmlImportException& xe)
		{
			rsDebug::printf(rsDebug::RS_VERBOSE,
			                "[VERBOSE] Antenna '%s' does not specify efficiency, assuming unity.\n", ant_name.c_str());
		}
		//Add it to the world
		world->Add(antenna);
	}

	/// Process a multipath surface and add it to the world
	void ProcessMultipath(const TiXmlHandle& mpXML, World* world)
	{
		//Get the reflecting factor
		const rsFloat factor = GetChildRsFloat(mpXML, "factor");
		const rsFloat nx = GetChildRsFloat(mpXML, "nx");
		const rsFloat ny = GetChildRsFloat(mpXML, "ny");
		const rsFloat nz = GetChildRsFloat(mpXML, "nz");
		const rsFloat d = GetChildRsFloat(mpXML, "d");
		//Create the multipath object
		MultipathSurface* mps = new MultipathSurface(nx, ny, nz, d, factor);
		//Add it to the world
		world->AddMultipathSurface(mps);
	}

	/// Process a timing source and add it to the world
	void ProcessTiming(const TiXmlHandle& antXML, World* world)
	{
		//Get the name of the antenna
		const string name = GetAttributeString(antXML, "name", "Timing sources must specify a name");
		PrototypeTiming* timing = new PrototypeTiming(name);
		//Process all the clock entries
		TiXmlHandle plat = antXML.ChildElement("noise_entry", 0);
		for (int i = 1; plat.Element() != 0; i++)
		{
			const rsFloat alpha = GetChildRsFloat(plat, "alpha");
			const rsFloat weight = GetChildRsFloat(plat, "weight");
			timing->AddAlpha(alpha, weight);
			plat = antXML.ChildElement("noise_entry", i);
		}
		// Process the frequency offset
		try
		{
			const rsFloat offset = GetChildRsFloat(antXML, "freq_offset");
			timing->AddFreqOffset(offset);
		}
		catch (XmlImportException& xe)
		{
		}
		try
		{
			const rsFloat stdev = GetChildRsFloat(antXML, "random_freq_offset");
			timing->AddRandomFreqOffset(stdev);
		}
		catch (XmlImportException& xe)
		{
		}
		// Process the phase offset
		try
		{
			const rsFloat offset = GetChildRsFloat(antXML, "phase_offset");
			timing->AddPhaseOffset(offset);
		}
		catch (XmlImportException& xe)
		{
		}
		try
		{
			const rsFloat stdev = GetChildRsFloat(antXML, "random_phase_offset");
			timing->AddRandomPhaseOffset(stdev);
		}
		catch (XmlImportException& xe)
		{
		}
		// Process the frequency
		try
		{
			const rsFloat freq = GetChildRsFloat(antXML, "frequency");
			timing->SetFrequency(freq);
		}
		catch (XmlImportException& xe)
		{
			//If there is no frequency, we default to the system sample frequency
			timing->SetFrequency(rsParameters::rate());
			rsDebug::printf(rsDebug::RS_VERBOSE,
			                "[VERBOSE] Clock section '%s' does not specify frequency. Assuming %g.\n", name.c_str(),
			                rsParameters::rate());
		}
		//Process the synconpulse tag
		if (bool sync = GetAttributeBool(antXML, "synconpulse", "", true))
		{
			timing->SetSyncOnPulse();
		}
		//Notify the debug log
		rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Loading timing source '%s'\n", name.c_str());

		//Add it to the world
		world->Add(timing);
	}

	/// Process the <parameters> element
	void ProcessParameters(const TiXmlHandle& root)
	{
		//Get the simulation start and end times
		rsParameters::modify_parms()->SetTime(GetChildRsFloat(root, "starttime"), GetChildRsFloat(root, "endtime"));
		//Get the propagation speed in air
		try
		{
			const rsFloat c = GetChildRsFloat(root, "c");
			rsParameters::modify_parms()->SetC(c);
		}
		catch (XmlImportException& xe)
		{
			rsDebug::printf(rsDebug::RS_VERBOSE, "[VERBOSE] Using default value of c: %f(m/s)\n", rsParameters::c());
		}
		//Get the export sampling rate
		try
		{
			const rsFloat rate = GetChildRsFloat(root, "rate");
			rsParameters::modify_parms()->SetRate(rate);
		}
		catch (XmlImportException& xe)
		{
			rsDebug::printf(rsDebug::RS_VERBOSE, "[VERBOSE] Using default sampling rate.\n");
		}
		//Get the cw Interpolation rate
		try
		{
			const rsFloat rate = GetChildRsFloat(root, "interprate");
			rsParameters::modify_parms()->SetCWSampleRate(rate);
		}
		catch (XmlImportException& xe)
		{
			rsDebug::printf(rsDebug::RS_VERBOSE,
			                "[VERBOSE] Using default value of CW position interpolation rate: %g\n",
			                rsParameters::cw_sample_rate());
		}
		//Get the random seed
		try
		{
			const rsFloat seed = GetChildRsFloat(root, "randomseed");
			rsParameters::modify_parms()->SetRandomSeed(static_cast<unsigned int>(std::fabs(seed)));
		}
		catch (XmlImportException& xe)
		{
			rsDebug::printf(rsDebug::RS_VERBOSE, "[VERBOSE] Using random seed from clock(): %d\n",
			                rsParameters::random_seed());
		}
		//Get the number of ADC bits to simulate
		try
		{
			const rsFloat adc_bits = GetChildRsFloat(root, "adc_bits");
			rsParameters::modify_parms()->SetADCBits(static_cast<unsigned int>(std::floor(adc_bits)));
			rsDebug::printf(rsDebug::RS_VERBOSE, "[VERBOSE] Quantizing results to %d bits\n", rsParameters::adc_bits());
		}
		catch (XmlImportException& xe)
		{
			rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VERBOSE] Using full precision simulation.\n");
		}
		// Get the oversampling ratio
		try
		{
			const rsFloat ratio = GetChildRsFloat(root, "oversample");
			rsParameters::modify_parms()->SetOversampleRatio(static_cast<unsigned int>(std::floor(ratio)));
		}
		catch (XmlImportException& xe)
		{
			rsDebug::printf(rsDebug::RS_VERY_VERBOSE,
			                "[VV] Oversampling not in use. Ensure than pulses are correctly sampled.\n");
		}
		//Process the "export" tag
		if (const TiXmlHandle exporttag = root.ChildElement("export", 0); exporttag.Element())
		{
			const bool export_xml = GetAttributeBool(exporttag, "xml", "", rsParameters::export_xml());
			const bool export_csv = GetAttributeBool(exporttag, "csv", "", rsParameters::export_csv());
			const bool export_binary = GetAttributeBool(exporttag, "binary", "", rsParameters::export_binary());
			rsParameters::modify_parms()->SetExporters(export_xml, export_csv, export_binary);
		}
	}

	/// Process the XML tree, starting at the root
	void ProcessDocument(const TiXmlHandle& root, World* world, bool included);

	/// Process the inclusion of a file
	void ProcessInclude(const TiXmlHandle& plat, World* world)
	{
		const char* name = GetNodeText(plat);
		TiXmlDocument doc(name);
		if (!doc.LoadFile())
		{
			throw std::runtime_error("Cannot open included file: " + std::string(name));
		}
		//Process the XML document
		const TiXmlHandle root(doc.RootElement());
		ProcessDocument(root, world, true);
	}

	/// Process the XML tree, starting at the root
	void ProcessDocument(const TiXmlHandle& root, World* world, const bool included)
	{
		if (!included)
		{
			//Process the parameters
			const TiXmlHandle parameters = root.ChildElement("parameters", 0);
			ProcessParameters(parameters);
		}
		//Process all the pulses
		TiXmlHandle plat = root.ChildElement("pulse", 0);
		for (int i = 1; plat.Element() != 0; i++)
		{
			ProcessPulse(plat, world);
			plat = root.ChildElement("pulse", i);
		}
		//Process all the antennas
		plat = root.ChildElement("antenna", 0);
		for (int i = 1; plat.Element() != 0; i++)
		{
			ProcessAntenna(plat, world);
			plat = root.ChildElement("antenna", i);
		}
		//Process all the timing sources
		plat = root.ChildElement("timing", 0);
		for (int i = 1; plat.Element() != 0; i++)
		{
			ProcessTiming(plat, world);
			plat = root.ChildElement("timing", i);
		}
		//Process all the multipath surfaces
		plat = root.ChildElement("multipath", 0);
		for (int i = 1; plat.Element() != 0; i++)
		{
			ProcessMultipath(plat, world);
			plat = root.ChildElement("multipath", i);
		}
		//Process all the platforms
		plat = root.ChildElement("platform", 0);
		for (int i = 1; plat.Element() != 0; i++)
		{
			ProcessPlatform(plat, world); //Recursively process the platform
			plat = root.ChildElement("platform", i);
		}
		//Process all the includes
		plat = root.ChildElement("include", 0);
		for (int i = 1; plat.Element() != 0; i++)
		{
			ProcessInclude(plat, world); //Recursively process the platform
			plat = root.ChildElement("include", i);
		}
		//Process all the incblocks
		plat = root.ChildElement("incblock", 0);
		for (int i = 1; plat.Element() != 0; i++)
		{
			ProcessDocument(plat, world, true); //Recursively process the platform
			plat = root.ChildElement("incblock", i);
		}
	}
} //Anonymous Namespace

/// Load a XML file into the world with the given filename
void xml::LoadXMLFile(const string& filename, World* world)
{
	TiXmlDocument doc(filename.c_str());
	if (!doc.LoadFile())
	{
		throw std::runtime_error("Cannot open script file");
	}
	//Process the XML document
	const TiXmlHandle root(doc.RootElement());
	ProcessDocument(root, world, false);
	//Create multipath duals of all objects, if a surface was added
	world->ProcessMultipath();
}
