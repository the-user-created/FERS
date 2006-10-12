//rsresponse.cpp 
//Implementation of ResponseBase and derived classes
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//3 August 2006

#include <sstream>
#include <fstream>
#include <iomanip>
#include "rsresponse.h"
#include "rsradar.h"

#define TIXML_USE_STL
#include "tinyxml/tinyxml.h"

using namespace rs;

namespace {

/// Attach a text node to an XML element
// For creating structures like this
//<node>
//<name>text</name>
//</node>
void AttachTextNode(TiXmlElement *root, std::string name, std::string text)
{
  TiXmlElement *element = new TiXmlElement(name);
  root->LinkEndChild(element);
  TiXmlText *xtext = new TiXmlText(text);
  element->LinkEndChild(xtext);
}

/// Attach a text node to an XML element, getting the text by converting a rsFloat to a string
// For creating structures like this
//<node>
//<name>text</name>
//</node>
void AttachRsFloatNode(TiXmlElement *root, std::string name, rsFloat data, bool scientific = true, int precision = 10) {
  
  std::ostringstream oss;
  if (scientific)
    oss.setf(std::ios::scientific);
  oss << std::setprecision(precision) << data;
  AttachTextNode(root, name, oss.str());
}

/// Attach a text node to an XML element, getting the text by converting an int to a string
// For creating structures like this
//<node>
//<name>text</name>
//</node>
void AttachIntNode(TiXmlElement *root, std::string name, int data) {
  
  std::ostringstream oss;
  oss << data;
  AttachTextNode(root, name, oss.str());
}

}

//
// ResponseBase Implementation
//

ResponseBase::ResponseBase(rsFloat start, const Transmitter* transmitter):
  start(start),
  transmitter(transmitter)
{
}

ResponseBase::~ResponseBase()
{
}

// Get the start time
rsFloat ResponseBase::GetStart() const
{
  return start;
}

/// Add a receiver deadtime pair (start of deadtime, end of deadtime)
void ResponseBase::AddDeadTime(std::pair<rsFloat, rsFloat> pair)
{
  
  deadtime.push_back(pair);
}

/// Render the deadtime to an XML file
void ResponseBase::RenderDeadtimeXML(TiXmlElement *root)
{

  if (!deadtime.empty()) {
    //Create a node for the response
    TiXmlElement *element = new TiXmlElement("Deadtime");
    root->LinkEndChild(element);
    //Attach nodes for properties of the response
    ::AttachIntNode(element, "segments", deadtime.size());
    //Loop through the segments, adding each in turn
    std::vector <std::pair<rsFloat, rsFloat> >::iterator i;
    for (i = deadtime.begin(); i != deadtime.end(); i++)
      {
	TiXmlElement *segment = new TiXmlElement("DeadtimeSegment");
	element->LinkEndChild(segment);
	::AttachRsFloatNode(segment, "start", (*i).first, false);
	::AttachRsFloatNode(segment, "end", (*i).second, false);
      }      
  }
}

//Get the name of the transmitter which caused this response
std::string ResponseBase::GetTransmitterName() const
{
  return transmitter->GetName();
}

//
// Response Implementation
//

/// Constructor
Response::Response(const rs::RadarWaveform *wave, rsFloat start, rsFloat amplitude, rsFloat phase, rsFloat doppler, rsFloat noise_temperature, const Transmitter *transmitter):
  ResponseBase(start, transmitter),
  wave(wave),
  amplitude(amplitude),
  phase(phase),
  doppler(doppler),
  noise_temperature(noise_temperature),
  range_error(false)
{
}

/// Destructor
Response::~Response()
{
}

/// Return the time the pulse's energy starts
rsFloat Response::StartTime() const
{
  return start;
}

/// Return the time the pulse's energy ends
rsFloat Response::EndTime() const
{
  return start+GetLength();
}

/// Return the length of the pulse
rsFloat Response::GetLength() const
{
  return wave->GetLength();
}

/// Render the response to an XML file
void Response::RenderXML(TiXmlElement *root)
{
    //Create a node for the response
  TiXmlElement *element = new TiXmlElement("Response");
  root->LinkEndChild(element);
  element->SetAttribute("transmitter", GetTransmitterName());
  //Attach nodes for properties of the response
  AttachRsFloatNode(element, "start", start, false);
  AttachRsFloatNode(element, "power", amplitude*wave->GetPower());
  AttachRsFloatNode(element, "amplitude", std::sqrt(amplitude*wave->GetPower()));
  AttachRsFloatNode(element, "Iamplitude", std::cos(phase)*std::sqrt(amplitude*wave->GetPower()));
  AttachRsFloatNode(element, "Qamplitude", std::sin(phase)*std::sqrt(amplitude*wave->GetPower()));
  AttachRsFloatNode(element, "doppler", wave->GetCarrier()*(1-doppler));
  AttachRsFloatNode(element, "length", GetLength());
  AttachRsFloatNode(element, "phase", phase);
  AttachRsFloatNode(element, "phasedeg", phase/M_PI*180);
  AttachRsFloatNode(element, "noise_temperature", noise_temperature);
  AttachTextNode(element, "name", wave->GetName());
  //Render the deadtime to the XML file
  RenderDeadtimeXML(element);
}

/// Render the response to an CSV file
void Response::RenderCSV(std::ofstream &of)
{
  of << GetStart() << ", " <<  amplitude*wave->GetPower() << ", " << phase << ", " << wave->GetCarrier()*(1-doppler) << "\n";
}

/// Render the response to an array
boost::shared_array<rsComplex> Response::RenderBinary(rsFloat& rate, unsigned int &size)
{
  const PulseWaveform *pw = dynamic_cast<const PulseWaveform*>(wave);
  rate = 0;
  //Fill the RenderParams struct with the render parameters
  RenderParams rp;
  rp.power = amplitude;
  rp.phase = phase;
  rp.doppler = doppler;
  rp.start = GetStart();
  rp.noise_temperature = noise_temperature;
  return pw->Render(rate, rp, size);
}

/// Return a pointer to the waveform
const rs::RadarWaveform* Response::GetWave() const
{
  return wave;
}

/// Return the Response type
ResponseBase::ResponseType Response::Type() const
{
  return ResponseBase::RS_PULSED;
}

//
// CWResponse Implementation
//

/// CWInterpPoint Constructor
CWInterpPoint::CWInterpPoint(rsFloat power, rsFloat delay, rsFloat doppler, rsFloat phase, rsFloat noise_temperature):
  power(power),
  time(delay),
  doppler(doppler),
  phase(phase),
  noise_temperature(noise_temperature)
{
}

/// Constructor
CWResponse::CWResponse(const rs::RadarWaveform *wave, rsFloat start, const Transmitter *transmitter):
  ResponseBase(start, transmitter),
  wave(wave)
{
}

/// Destructor
CWResponse::~CWResponse()
{
  points.clear();
}

/// Return the length of the pulse
rsFloat CWResponse::GetLength() const
{
  return wave->GetLength();
}

/// Render a single response point to XML
void CWResponse::RenderResponseXML(TiXmlElement *root, const CWInterpPoint &point)
{
  //Create a node for the response
  TiXmlElement *element = new TiXmlElement("InterpolationPoint");
  root->LinkEndChild(element);
  //Attach nodes for properties of the response
  AttachRsFloatNode(element, "amplitude", std::sqrt(point.power), false);
  AttachRsFloatNode(element, "phase", point.phase, false);
  AttachRsFloatNode(element, "doppler", wave->GetCarrier()*(1-point.doppler), false);
  AttachRsFloatNode(element, "power", point.power*wave->GetPower());
  AttachRsFloatNode(element, "Iamplitude", std::cos(point.phase)*std::sqrt(point.power*wave->GetPower()));
  AttachRsFloatNode(element, "Qamplitude", std::sin(point.phase)*std::sqrt(point.power*wave->GetPower()));
  AttachRsFloatNode(element, "noise_temperature", point.noise_temperature);
}

/// Render the response to an XML file
void CWResponse::RenderXML(TiXmlElement *root)
{
  //Create a node for the response
  TiXmlElement *element = new TiXmlElement("CWResponse");
  root->LinkEndChild(element);
  element->SetAttribute("transmitter", GetTransmitterName());

  //Attach nodes for properties of the response
  ::AttachRsFloatNode(element, "start", start, false);
  AttachTextNode(element, "name", wave->GetName());

  //Render each interpolation point in turn
  std::vector<CWInterpPoint>::iterator i;
  for (i = points.begin(); i != points.end(); i++)
    RenderResponseXML(element, *i);

  //Render the deadtime
  RenderDeadtimeXML(element);

}

/// Render a CWInterpPoint as CSV
void CWResponse::RenderResponseCSV(std::ofstream &of, const CWInterpPoint &point)
{
  of << point.time << ", " << point.power << ", " << point.phase << ", " << wave->GetCarrier()*(1-point.doppler) << "\n";
}

/// Render the response to a CSV file
void CWResponse::RenderCSV(std::ofstream &of)
{
  //Render each interpolation point
  std::vector<CWInterpPoint>::const_iterator i;
  for (i = points.begin(); i != points.end(); i++)
    RenderResponseCSV(of, *i);
}

/// Render the response to an array
boost::shared_array<rsComplex> CWResponse::RenderBinary(rsFloat& rate, unsigned int &size)
{
  const CWWaveform *cww = dynamic_cast<const CWWaveform*>(wave);
  return cww->Render(points, size);  
}

/// Return a pointer to the waveform
const rs::RadarWaveform* CWResponse::GetWave() const
{
  return wave;
}

/// Add an interp point to the vector
void CWResponse::AddInterpPoint(CWInterpPoint &point)
{
  // This method does not need a mutex as only one thread owns any non-const CWResponse object
  points.push_back(point);
}

/// Return the Response type
ResponseBase::ResponseType CWResponse::Type() const
{
  return ResponseBase::RS_CONTINUOUS;
}

//Return a const reference to the interpolation points
const std::vector<CWInterpPoint>& CWResponse::GetPoints() const
{
  return points;
}

