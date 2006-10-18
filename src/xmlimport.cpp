//xmlimport.cpp
//Import a simulator world and simulation parameters from an XML file
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started 26 April 2006

//TODO: Rewrite this code to be less ugly

#define TIXML_USE_STL //Tell tinyxml to use the STL instead of it's own string class

#include <string>
#include <sstream>
#include <stdexcept>
#include <cmath>

#include "xmlimport.h"
#include "tinyxml/tinyxml.h"
#include "rsdebug.h"
#include "rsworld.h"
#include "rsplatform.h"
#include "rstarget.h"
#include "rsradar.h"
#include "rsportable.h"
#include "rspulsefactory.h"
#include "rsparameters.h"
#include "rsantennafactory.h"
#include "rsantenna.h"
#include "rstiming.h"

using namespace rs;
using std::string;

namespace {

/// Exception for reporting an XML parsing error
class XmlImportException: public std::runtime_error {
public:
  XmlImportException(std::string error):
    std::runtime_error("[ERROR] Error while parsing XML file: "+error)
  {} 
};
 
/// Function which takes a TiXmlHandle and returns the text contained in it's children.
//For processing XML like this:
//<tree>
//<leaf1>Green</leaf1>
//<leaf2>Blue</leaf2>
//</tree>
//Pass a handle to tree, and the string "leaf1" to get "Green"
const char* GetChildText(TiXmlHandle &parent, const char *childname)
{
  TiXmlHandle tmp = parent.FirstChildElement(childname);
  if (tmp.Element() == 0)
    return 0; //The element does not exist
  //Return the text
  return tmp.Element()->GetText();
}

/// Gets child text as a rsFloat. See GetChildText for usage description
rsFloat GetChildRsFloat(TiXmlHandle &parent, const char *childname)
{
  const char* data = GetChildText(parent, childname);
  //If there is no data, flag failure and return
  if (!data)
    throw XmlImportException("No data in child element "+string(childname)+" during GetChildRsFloat.");
  //Parse the first rsFloat from the data
  rsFloat result;
  std::istringstream iss(data);
  iss >> result;
  return result;
}

/// Gets the text content of a node as an rsFloat.
//For XML like this:
//<rcs>10</rcs>
rsFloat GetNodeFloat(TiXmlHandle &node)
{
  if (!node.Element())
    throw XmlImportException("[BUG] Node does not exist during GetNodeFloat");
  const char *data = node.Element()->GetText();
  if (!data)
    throw XmlImportException("Node does not contain text during GetNodeFloat");
  rsFloat result;
  std::istringstream iss(data);
  iss >> result;
  return result;
}

/// Return the string associated with an attribute or throw an exception on failure
std::string GetAttributeString(TiXmlHandle &handle, std::string name, std::string error, bool optional=false)
{
  const char *text = handle.Element()->Attribute(name);
  if (text)
    return string(text);
  else {
    if (!optional)
      throw XmlImportException(error);
    else
      return string("");
  }
}

/// Return the bool associated with an attribute
bool GetAttributeBool(TiXmlHandle &handle, std::string name, std::string error, bool def)
{
  string str = GetAttributeString(handle, name, error, true);
  if (str == "")
    return def;
  return (str == "true");
}

/// Process a target XML entry
void ProcessTarget(TiXmlHandle &targXML, Platform *platform, World *world)
{
  DEBUG_PRINT(rsDebug::RS_VERY_VERBOSE, "[VV] Loading Target");
  string name = GetAttributeString(targXML, "name", "Target does not specify a name");
  Target *target = new Target(platform, name);
  //Get the RCS
  TiXmlHandle rcsXML = targXML.ChildElement("rcs", 0);
  if (!rcsXML.Element())
    throw XmlImportException("Target "+name+" does not specify RCS.");
  string rcs_type = GetAttributeString(rcsXML, "type", "RCS attached to target "+name+" does not specify type.");
  if (rcs_type == "isotropic") {
    TiXmlHandle rcsValueXML = rcsXML.ChildElement("value", 0);
    if (!rcsValueXML.Element())
      throw XmlImportException("Target "+name+" does not specify value of isotropic RCS.");
    rsFloat value = GetNodeFloat(rcsValueXML);
    target->SetRCS(value, Target::RCS_ISOTROPIC);
  }
  else {
    throw XmlImportException("RCS type "+rcs_type+" not currently supported.");
  }
  //Add the target to the world
  world->Add(target);
}

/// Process a receiver XML entry
Receiver *ProcessReceiver(TiXmlHandle &recvXML, Platform *platform, World *world)
{
  DEBUG_PRINT(rsDebug::RS_VERY_VERBOSE, "[VV] Loading Receiver");

  //Get the name of the receiver
  string name = GetAttributeString(recvXML, "name", "Receiver does not specify a name");
  Receiver *receiver = new Receiver(platform, name);

  //Get the name of the antenna
  string ant_name = GetAttributeString(recvXML, "antenna", "Receiver " + string(name) + " does not specify an antenna");
  
  Antenna *antenna = world->FindAntenna(ant_name);
  if (!antenna)
    throw XmlImportException("Antenna with name " + ant_name + " does not exist when processing Receiver " + string(name));
  //Set the receiver's antenna
  receiver->SetAntenna(antenna);

  //Process the noise temperature tag
  try {
    rsFloat temperature;
    temperature = GetChildRsFloat(recvXML, "noise_temp");
    receiver->SetNoiseTemperature(temperature);
  } 
  catch (XmlImportException e) {
  }

  //Process the PRF tag
  rsFloat prf = GetChildRsFloat(recvXML, "prf");
  rsFloat skip = GetChildRsFloat(recvXML, "window_skip");
  rsFloat length = GetChildRsFloat(recvXML, "window_length");
  receiver->SetWindowProperties(length, prf, skip);

  //Get the name of the timing source
  string timing_name = GetAttributeString(recvXML, "timing", "Receiver "+name+" does not specify a timing source");
  Timing *timing = world->FindTiming(timing_name);
  if (!timing)
    throw XmlImportException("Timing source " + timing_name + " does not exist when processing receiver "+name);
  receiver->SetTiming(timing);

  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Loading receiver %s\n", receiver->GetName().c_str());
  //Add the receiver to the world
  world->Add(receiver);

  return receiver;
}

/// Create a PulseTransmitter object and process XML entry
Transmitter *ProcessPulseTransmitter(TiXmlHandle &transXML, std::string &name, Platform *platform, World *world)
{
  PulseTransmitter *transmitter = new PulseTransmitter(platform, string(name));
  //Get the name of the pulse
  string pulse_name = GetAttributeString(transXML, "pulse", "Transmitter " + name + " does not specify a pulse");
  //Get the pulse from the table of pulses
  RadarWaveform *wave = world->FindPulse(pulse_name);
  if (!wave)
    throw XmlImportException("Pulse with name " + pulse_name + " does not exist");
  //Get the Pulse Repetition Frequency
  rsFloat prf = GetChildRsFloat(transXML, "prf");
  //Attach the pulse to the transmitter
  transmitter->SetWave(wave);
  transmitter->SetPRF(prf);
  return transmitter;
}

/// Create a PulseTransmitter object and process XML entry
Transmitter *ProcessCWTransmitter(TiXmlHandle &transXML, std::string &name, Platform *platform, World *world)
{
  CWTransmitter *transmitter = new CWTransmitter(platform, string(name));
  //Get the name of the pulse
  string pulse_name = GetAttributeString(transXML, "pulse", "Transmitter " + name + " does not specify a pulse");
  //Get the pulse from the table of pulses
  RadarWaveform *wave = world->FindCWWaveform(pulse_name);
  if (!wave)
    throw XmlImportException("Pulse with name " + pulse_name + " does not exist");
  //Attach the CW waveform to the transmitter
  transmitter->SetWave(wave);
  return transmitter;
}

/// Process a transmitter XML entry
Transmitter *ProcessTransmitter(TiXmlHandle &transXML, Platform *platform, World *world)
{
  DEBUG_PRINT(rsDebug::RS_VERY_VERBOSE, "[VV] Loading Transmitter");

  //Get the name of the transmitter
  string name = GetAttributeString(transXML, "name", "Transmitter does not specify a name");


  //Get the transmitter type
  string type = GetAttributeString(transXML, "type", "Transmitter "+name+" does not specify type");
  Transmitter *transmitter;
  if (type == "pulsed")
    transmitter = ProcessPulseTransmitter(transXML, name, platform, world);
  else if (type == "continuous")
    transmitter = ProcessCWTransmitter(transXML, name, platform, world);
  else
    throw XmlImportException("[ERROR] Invalid transmitter type specifed in transmitter "+name);

  //Get the name of the antenna
  string ant_name = GetAttributeString(transXML, "antenna", "Transmitter " + name + " does not specify an antenna");
  Antenna *antenna = world->FindAntenna(ant_name);
  if (!antenna)
    throw XmlImportException("Antenna with name " + ant_name + " does not exist when processing Transmitter " + string(name));
  //Set the transmitter's antenna
  transmitter->SetAntenna(antenna);

  //Get the name of the timing source
  string timing_name = GetAttributeString(transXML, "timing", "Transmitter "+name+" does not specify a timing source");
  Timing *timing = world->FindTiming(timing_name);
  if (!timing)
    throw XmlImportException("Timing source " + timing_name + " does not exist when processing transmitter "+name);
  transmitter->SetTiming(timing);
  
  //Add the transmitter to the world
  world->Add(transmitter);

  return transmitter;
}

/// Process a monostatic (Receiver and Transmitter sharing an antenna)
void ProcessMonostatic(TiXmlHandle &transXML, Platform *platform, World *world)
{
  Transmitter *trans = ProcessTransmitter(transXML, platform, world);
  Receiver *recv = ProcessReceiver(transXML, platform, world);
  trans->MakeMonostatic(recv);
  recv->MakeMonostatic(trans);
}

/// Process a motion path waypoint
void ProcessWaypoint(TiXmlHandle &handXML, Path *path)
{
  try {
    rsFloat x, y, z, t;
    x = GetChildRsFloat(handXML, "x");
    y = GetChildRsFloat(handXML, "y");
    z = GetChildRsFloat(handXML, "altitude");
    t = GetChildRsFloat(handXML, "time");
    Coord coord;
    coord.t = t;
    coord.pos = Vec3(x, y, z);
    path->AddCoord(coord);
  }
  catch (XmlImportException &e) {
    DEBUG_PRINT(rsDebug::RS_VERBOSE, "[WARNING] Parse Error While Importing Waypoint. Discarding Waypoint.");
  }
}

/// Process a MotionPath XML entry
void ProcessMotionPath(TiXmlHandle &mpXML, Platform *platform)
{
  DEBUG_PRINT(rsDebug::RS_VERY_VERBOSE, "[VV] Loading Motion Path"); 
  //Get a pointer to the platform's path
  Path *path = platform->GetMotionPath();
  //Get the interpolation type
  try {
    std::string rottype = GetAttributeString(mpXML, "interpolation", "");
    if (rottype == "linear")
      path->SetInterp(Path::RS_INTERP_LINEAR);
    else if (rottype == "cubic")
      path->SetInterp(Path::RS_INTERP_CUBIC);
    else if (rottype == "static")
      path->SetInterp(Path::RS_INTERP_STATIC);
    else {
      rsDebug::printf(rsDebug::RS_VERBOSE, "[WARNING] Unsupported motion path interpolation type for platform "+platform->GetName()+". Defaulting to static.\n");
      path->SetInterp(Path::RS_INTERP_STATIC);
    }
  }
  catch (XmlImportException e) {
    rsDebug::printf(rsDebug::RS_VERBOSE, "[WARNING] Motion path interpolation type not specified for platform "+platform->GetName()+". Defaulting to static.\n");
    path->SetInterp(Path::RS_INTERP_STATIC);
  } 

  //Process all the PositionWaypoints
  TiXmlHandle tmp = mpXML.ChildElement("positionwaypoint", 0);
  for (int i = 1; tmp.Element() != 0; i++) {
    ProcessWaypoint(tmp, path);
    tmp = mpXML.ChildElement("positionwaypoint", i);
  }
  //Finalise the path after all the waypoints have been loaded
  path->Finalize();
}

/// Process a rotation path waypoint
void ProcessRotationWaypoint(TiXmlHandle &handXML, RotationPath *path)
{
  try {
    RotationCoord coord;
    coord.elevation = GetChildRsFloat(handXML, "elevation");
    coord.azimuth = GetChildRsFloat(handXML, "azimuth");
    coord.t = GetChildRsFloat(handXML, "time");
    path->AddCoord(coord);
  }
  catch (XmlImportException &e) {
    DEBUG_PRINT(rsDebug::RS_VERBOSE, "[WARNING] Parse Error While Importing Waypoint. Discarding Waypoint.");
  }
}

/// Process Waypoints for RotationPath
void ProcessRotationWaypoints(TiXmlHandle &mpXML, RotationPath *path)
{
  //Process all the RotationWaypoints
  TiXmlHandle tmp = mpXML.ChildElement("rotationwaypoint", 0);
  for (int i = 1; tmp.Element() != 0; i++) {
    ProcessRotationWaypoint(tmp, path);
    tmp = mpXML.ChildElement("rotationwaypoint", i);
  }
  //Finalise the path after all the waypoints have been loaded
  path->Finalize();
}

/// Process an entry for a fixed rotation
void ProcessRotationConstant(TiXmlHandle &mpXML, Platform* platform)
{
  RotationPath* path = platform->GetRotationPath();
  try {
    RotationCoord start, rate;
    start.azimuth = GetChildRsFloat(mpXML, "startazimuth");
    start.elevation = GetChildRsFloat(mpXML, "startelevation");
    rate.azimuth = GetChildRsFloat(mpXML, "azimuthrate");
    rate.elevation = GetChildRsFloat(mpXML, "elevationrate");
    path->SetConstantRate(start, rate);
  }
  catch (XmlImportException &e) {
    DEBUG_PRINT(rsDebug::RS_VERBOSE, "[WARNING] Parse Error While Importing Constant Rotation.");
  }
}

/// Process a RotationPath XML entry
void ProcessRotationPath(TiXmlHandle &mpXML, Platform *platform)
{
  DEBUG_PRINT(rsDebug::RS_VERY_VERBOSE, "[VV] Loading Rotation Path");
  
  
  
  //Get a pointer to the rotation path
  RotationPath *path = platform->GetRotationPath();

  //Get the interpolation type
  try {
    std::string rottype = GetAttributeString(mpXML, "interpolation", "");
    if (rottype == "linear")
      path->SetInterp(RotationPath::RS_INTERP_LINEAR);
    else if (rottype == "cubic")
      path->SetInterp(RotationPath::RS_INTERP_CUBIC);
    else if (rottype == "static")
      path->SetInterp(RotationPath::RS_INTERP_STATIC);
    else {
      rsDebug::printf(rsDebug::RS_VERBOSE, "[WARNING] Unsupported rotation path interpolation type for platform "+platform->GetName()+". Defaulting to static.\n");
      path->SetInterp(RotationPath::RS_INTERP_STATIC);
    }
  }
  catch (XmlImportException e) {
    rsDebug::printf(rsDebug::RS_VERBOSE, "[WARNING] Rotation path interpolation type not specified for platform "+platform->GetName()+". Defaulting to static.\n");
    path->SetInterp(RotationPath::RS_INTERP_STATIC);
  }
  // Process the rotation waypoints
  ProcessRotationWaypoints(mpXML, path);
}

/// Process a platform, recursively processing all the elements that are attached to it
void ProcessPlatform(TiXmlHandle &platXML, World *world)
{
  Platform *platform;
  //Create the platform, using the name from the element
  const char *name = platXML.Element()->Attribute("name");
  if (name)
    platform = new Platform(string(name));
  else
    platform = new Platform(); //Use the default name, if one is set
  //Add the platform to the world
  world->Add(platform);

  //Process all the targets attached to the platform
  TiXmlHandle tmp = platXML.ChildElement("target", 0);
  for (int i = 1; tmp.Element() != 0; i++) {
    ProcessTarget(tmp, platform, world);
    tmp = platXML.ChildElement("target", i);
  }

  //Process all the receivers attached to the platform
  tmp = platXML.ChildElement("receiver", 0);
  for (int i = 1; tmp.Element() != 0; i++) {
    ProcessReceiver(tmp, platform, world);
    tmp = platXML.ChildElement("receiver", i);
  }

  //Process all the transmitters attached to the platform
  tmp = platXML.ChildElement("transmitter", 0);
  for (int i = 1; tmp.Element() != 0; i++) {
    ProcessTransmitter(tmp, platform, world);
    tmp = platXML.ChildElement("transmitter", i);
  }

  //Process all the monostatics attached to the platform
  tmp = platXML.ChildElement("monostatic", 0);
  for (int i = 1; tmp.Element() != 0; i++) {
    ProcessMonostatic(tmp, platform, world);
    tmp = platXML.ChildElement("monostatic", i);
  }

  //Process all the motion paths attached to the platform  
  tmp = platXML.ChildElement("motionpath", 0);
  for (int i = 1; tmp.Element() != 0; i++) {
    ProcessMotionPath(tmp, platform);
    tmp = platXML.ChildElement("motionpath", i);
  }

  //Process all the rotation paths attached to the platform
  tmp = platXML.ChildElement("rotationpath", 0);
  for (int i = 1; tmp.Element() != 0; i++) {
    ProcessRotationPath(tmp, platform);
    tmp = platXML.ChildElement("rotationpath", i);
  }

  //Process all the rotation paths attached to the platform
  tmp = platXML.ChildElement("fixedrotation", 0);
  for (int i = 1; tmp.Element() != 0; i++) {
    ProcessRotationConstant(tmp, platform);
    tmp = platXML.ChildElement("fixedrotation", i);
  }
}  

/// Process a pulse entry of type rect
void ProcessRectPulse(TiXmlHandle &pulseXML, World *world, std::string name)
{
  rsFloat length = GetChildRsFloat(pulseXML, "length");
  rsFloat power = GetChildRsFloat(pulseXML, "power");
  rsFloat carrier = GetChildRsFloat(pulseXML, "carrier");
  RadarWaveform *wave = rsPulseFactory::GenerateRectPulse(name, length, power, carrier);
  world->Add(wave);
}

/// Process a pulse entry of type rect
void ProcessRectAnyPulse(TiXmlHandle &pulseXML, World *world, std::string name)
{
  rsFloat length = GetChildRsFloat(pulseXML, "length");
  rsFloat power = GetChildRsFloat(pulseXML, "power");
  rsFloat carrier = GetChildRsFloat(pulseXML, "carrier");
  RadarWaveform *wave = rsPulseFactory::GenerateRectAnyPulse(name, length, power, carrier);
  world->Add(wave);
}

/// Process a pulse entry of type rect
void ProcessAnyPulseFile(TiXmlHandle &pulseXML, World *world, std::string name)
{
  string filename = GetAttributeString(pulseXML, "filename", "Pulse must specify a filename");
  rsFloat carrier = GetChildRsFloat(pulseXML, "carrier");
  rsFloat power = GetChildRsFloat(pulseXML, "power");
  RadarWaveform *wave = rsPulseFactory::GenerateAnyPulseFromFile(name, filename, power, carrier);
  world->Add(wave);
}

/// Process a pulse entry
void ProcessPulse(TiXmlHandle &pulseXML, World *world)
{
  //Get the name of the pulse
  string pulse_name = GetAttributeString(pulseXML, "name", "Pulses must specify a name");
  //Get the type of the pulse
  string pulse_type = GetAttributeString(pulseXML, "type", "Pulses must specify a type");
  //Generate the pulse    
  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Generating Pulse %s of type %s\n", pulse_name.c_str(), pulse_type.c_str());
  if (pulse_type == "rect")
    ProcessRectPulse(pulseXML, world, pulse_name);
  else if (pulse_type == "file")
    ProcessAnyPulseFile(pulseXML, world, pulse_name);
  else if (pulse_type == "rect_fd")
    ProcessRectAnyPulse(pulseXML, world, pulse_name);
  else
    throw XmlImportException("Unrecognised type in pulse");
}

/// Process a monochrome CW waveform
void ProcessCWMonochrome(TiXmlHandle &pulseXML, World *world, std::string name)
{
  rsFloat power = GetChildRsFloat(pulseXML, "power");
  rsFloat freq = GetChildRsFloat(pulseXML, "carrier");
  CWWaveform *cww  = rsPulseFactory::GenerateMonoCW(name, power, freq);
  world->Add(cww);
}

/// Process a CW waveform entry
void ProcessCWWaveform(TiXmlHandle &waveXML, World *world)
{
  //Get the name of the pulse
  string wave_name = GetAttributeString(waveXML, "name", "CW Waveforms must specify a name");
  //Get the type of the pulse
  string wave_type = GetAttributeString(waveXML, "type", "CW Waveforms must specify a type");
  //Generate the pulse    
  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Generating CW Waveform %s of type %s\n", wave_name.c_str(), wave_type.c_str());
  if (wave_type == "mono")
    ProcessCWMonochrome(waveXML, world, wave_name);
  else
    throw XmlImportException("Unrecognised type in CW Waveform");
}

Antenna *ProcessSincAntenna(TiXmlHandle &antXML, string name)
{
  rsFloat alpha = GetChildRsFloat(antXML, "alpha");
  rsFloat beta = GetChildRsFloat(antXML, "beta");
  rsFloat gamma = GetChildRsFloat(antXML, "gamma");
  return rs::CreateSincAntenna(name, alpha, beta, gamma);
}

Antenna *ProcessParabolicAntenna(TiXmlHandle &antXML, string name)
{
  rsFloat diameter = GetChildRsFloat(antXML, "diameter");
  return rs::CreateParabolicAntenna(name, diameter);
}

void ProcessAntenna(TiXmlHandle &antXML, World *world)
{
  //Get the name of the antenna
  string ant_name = GetAttributeString(antXML, "name", "Antennas must specify a name");
  //Get the type of the antenna
  string ant_pattern = GetAttributeString(antXML, "pattern", "Antennas must specify a pattern");
  Antenna *antenna;
  if (ant_pattern == "isotropic")
    antenna = CreateIsotropicAntenna(ant_name);
  else if (ant_pattern == "sinc")
    antenna = ProcessSincAntenna(antXML, ant_name);
  else if (ant_pattern == "parabolic")
    antenna = ProcessParabolicAntenna(antXML, ant_name);
  else
    throw XmlImportException("Antenna specified unrecognised gain pattern " + ant_pattern);
  //Notify the debug log
  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Loading antenna %s of type %s\n", ant_name.c_str(), ant_pattern.c_str());
  //Load the efficiency factor
  try {
    rsFloat factor = GetChildRsFloat(antXML, "efficiency");
    antenna->SetEfficiencyFactor(factor);
  } catch (XmlImportException &xe) {
    rsDebug::printf(rsDebug::RS_VERBOSE, "[VERBOSE] Antenna %s does not specify efficiency, assuming unity.", ant_name.c_str());
  }
  //Add it to the world
  world->Add(antenna);
}

/// Process a timing source and add it to the world
void ProcessTiming(TiXmlHandle &antXML, World *world)
{
  //Get the name of the antenna
  string name = GetAttributeString(antXML, "name", "Timing sources must specify a name");
  rsFloat jitter = GetChildRsFloat(antXML, "jitter");
  rsFloat rate = GetChildRsFloat(antXML, "frequency");

  Timing *timing = new Timing(name, rate, jitter);

  //Notify the debug log
  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "[VV] Loading timing source %s\n", name.c_str());

  //Add it to the world
  world->Add(timing);
}

/// Process the <parameters> element
void ProcessParameters(TiXmlHandle &root)
{
  //Get the simulation start and end times
  rsParameters::modify_parms()->SetTime(GetChildRsFloat(root, "starttime"), GetChildRsFloat(root, "endtime"));
  //Get the propagation speed in air
  try {
    rsFloat c = GetChildRsFloat(root, "c");
    rsParameters::modify_parms()->SetC(c);
  }
  catch (XmlImportException &xe)
    {
      rsDebug::printf(rsDebug::RS_VERBOSE, "[VERBOSE] Using default value of c: %d(m/s)\n", rsParameters::c());
    }
  //Get the export sampling rate
  try {
    rsFloat rate = GetChildRsFloat(root, "rate");
    rsParameters::modify_parms()->SetRate(rate);
      rsDebug::printf(rsDebug::RS_VERBOSE, "[VERBOSE] Using sampling rate %f\n", rate);
  }
  catch (XmlImportException &xe)
    {
      rsDebug::printf(rsDebug::RS_VERBOSE, "[VERBOSE] Using default sampling rate.\n");
    }
  //Get the cw Interpolation rate
  try {
    rsFloat rate = GetChildRsFloat(root, "interprate");
    rsParameters::modify_parms()->SetCWSampleRate(rate);
  }
  catch (XmlImportException &xe)
    {
      rsDebug::printf(rsDebug::RS_VERBOSE, "[VERBOSE] Using default value of CW position interpolation rate: %d\n", rsParameters::cw_sample_rate());
    }
  //Get the random seed
  try {
    rsFloat seed = GetChildRsFloat(root, "randomseed");
    rsParameters::modify_parms()->SetRandomSeed(static_cast<unsigned int>(std::fabs(seed)));
  }
  catch (XmlImportException &xe)
    {
      rsDebug::printf(rsDebug::RS_VERBOSE, "[VERBOSE] Using random seed from clock(): %d\n", rsParameters::random_seed());
    }
  //Process the "export" tag
  TiXmlHandle exporttag = root.ChildElement("export", 0);
  if (exporttag.Element()) {
    bool export_xml = GetAttributeBool(exporttag, "xml", "", rsParameters::export_xml());
    bool export_csv = GetAttributeBool(exporttag, "csv", "", rsParameters::export_csv());
    bool export_binary = GetAttributeBool(exporttag, "binary", "", rsParameters::export_binary());
    bool export_csvbinary = GetAttributeBool(exporttag, "csvbinary", "", rsParameters::export_csvbinary());
    rsParameters::modify_parms()->SetExporters(export_xml, export_csv, export_binary, export_csvbinary);
  }
}

/// Process the XML tree, starting at the root
void ProcessDocument(TiXmlHandle &root, World *world)
{  
  //Process the parameters
  TiXmlHandle parameters = root.ChildElement("parameters", 0);
  ProcessParameters(parameters);
  //Process all the pulses
  TiXmlHandle plat = root.ChildElement("pulse", 0);
  for (int i = 1; plat.Element() != 0; i++) {
    ProcessPulse(plat, world);
    plat = root.ChildElement("pulse", i);
  }
  //Process all CW waveforms
  plat = root.ChildElement("waveform", 0);
  for (int i = 1; plat.Element() != 0; i++) {
    ProcessCWWaveform(plat, world);
    plat = root.ChildElement("waveform", i);
  }
  //Process all the antennas
  plat = root.ChildElement("antenna", 0);
  for (int i = 1; plat.Element() != 0; i++) {
    ProcessAntenna(plat, world);
    plat = root.ChildElement("antenna", i);
  }
  //Process all the timing sources
  plat = root.ChildElement("timing", 0);
  for (int i = 1; plat.Element() != 0; i++) {
    ProcessTiming(plat, world);
    plat = root.ChildElement("timing", i);
  }
  //Process all the platforms
  plat = root.ChildElement("platform", 0);
  for (int i = 1; plat.Element() != 0; i++) {
    ProcessPlatform(plat, world); //Recursively process the platform
    plat = root.ChildElement("platform", i);
  }

}

}//Anonymous Namespace

/// Load a XML file into the world with the given filename
void xml::LoadXMLFile(string filename, World *world)
{
  TiXmlDocument doc(filename.c_str());
  if (!doc.LoadFile())
    throw std::runtime_error("Cannot open script file");
  //Process the XML document
  TiXmlHandle root(doc.RootElement());
  ::ProcessDocument(root, world);
}
