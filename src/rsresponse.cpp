//rsresponse.cpp
//Implementation of ResponseBase and derived classes
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//3 August 2006

#define TIXML_USE_STL

#include "rsresponse.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <tinyxml.h>

#include "rsradar.h"

using namespace rs;

namespace
{
	/// Attach a text node to an XML element
	// For creating structures like this
	//<node>
	//<name>text</name>
	//</node>
	void attachTextNode(TiXmlElement* root, const std::string& name, const std::string& text)
	{
		std::unique_ptr<TiXmlElement> element = std::make_unique<TiXmlElement>(name);
		std::unique_ptr<TiXmlText> xtext = std::make_unique<TiXmlText>(text);
		element->LinkEndChild(xtext.release());
		root->LinkEndChild(element.release());
	}

	/// Attach a text node to an XML element, getting the text by converting a rsFloat to a string
	// For creating structures like this
	//<node>
	//<name>text</name>
	//</node>
	void attachRsFloatNode(TiXmlElement* root, const std::string& name, const rsFloat data,
	                       const bool scientific = true)
	{
		std::ostringstream oss;
		if (scientific)
		{
			oss.setf(std::ios::scientific);
		}
		constexpr int precision = 10;
		oss << std::setprecision(precision) << data;
		attachTextNode(root, name, oss.str());
	}

	/// Attach a text node to an XML element, getting the text by converting an int to a string
	// For creating structures like this
	//<node>
	//<name>text</name>
	//</node>
	void attachIntNode(TiXmlElement* root, const std::string& name, const int data)
	{
		std::ostringstream oss;
		oss << data;
		attachTextNode(root, name, oss.str());
	}
}

//
// Interppoint Implementation
//
/// InterpPoint Constructor
InterpPoint::InterpPoint(const rsFloat power, const rsFloat start, const rsFloat delay, const rsFloat doppler,
                         const rsFloat phase, const rsFloat noiseTemperature):
	power(power),
	time(start),
	delay(delay),
	doppler(doppler),
	phase(phase),
	noise_temperature(noiseTemperature)
{
}

//
// ResponseBase Implementation
//

Response::Response(const RadarSignal* wave, const Transmitter* transmitter):
	_transmitter(transmitter),
	_wave(wave)
{
}

Response::~Response()
{
}


/// Return the time the pulse's energy starts
rsFloat Response::startTime() const
{
	if (_points.empty())
	{
		return 0;
	}
	return _points.front().time;
}

/// Return the time the pulse's energy ends
rsFloat Response::endTime() const
{
	if (_points.empty())
	{
		return 0;
	}
	return _points.back().time;
}

/// Return the length of the pulse
rsFloat Response::getLength() const
{
	return endTime() - startTime();
}

//Get the name of the transmitter which caused this response
std::string Response::getTransmitterName() const
{
	return _transmitter->getName();
}

/// Return a pointer to the waveform
const rs::RadarSignal* Response::getWave() const
{
	return _wave;
}

/// Render a single response point to XML
void Response::renderResponseXml(TiXmlElement* root, const InterpPoint& point) const
{
	// Create a node for the response
	std::unique_ptr<TiXmlElement> element = std::make_unique<TiXmlElement>("InterpolationPoint");
	root->LinkEndChild(element.get());
	// Attach nodes for properties of the response
	attachRsFloatNode(element.get(), "time", point.time, false);
	attachRsFloatNode(element.get(), "amplitude", std::sqrt(point.power * _wave->getPower()), false);
	attachRsFloatNode(element.get(), "phase", point.phase, false);
	attachRsFloatNode(element.get(), "doppler", _wave->getCarrier() * (1 - point.doppler), false);
	attachRsFloatNode(element.get(), "power", point.power * _wave->getPower());
	attachRsFloatNode(element.get(), "Iamplitude", std::cos(point.phase) * std::sqrt(point.power * _wave->getPower()));
	attachRsFloatNode(element.get(), "Qamplitude", std::sin(point.phase) * std::sqrt(point.power * _wave->getPower()));
	attachRsFloatNode(element.get(), "noise_temperature", point.noise_temperature);
	attachRsFloatNode(element.get(), "phasedeg", point.phase / M_PI * 180);
	element.release(); // Release ownership to avoid deletion NOLINT (must release and disregard the returned pointer)
}

/// Render the response to an XML file
void Response::renderXml(TiXmlElement* root)
{
	// Create a node for the response
	std::unique_ptr<TiXmlElement> element = std::make_unique<TiXmlElement>("Response");
	root->LinkEndChild(element.get());
	element->SetAttribute("transmitter", getTransmitterName());

	// Attach nodes for properties of the response
	::attachRsFloatNode(element.get(), "start", startTime(), false);
	attachTextNode(element.get(), "name", _wave->getName());

	// Render each interpolation point in turn
	for (std::vector<InterpPoint>::iterator i = _points.begin(); i != _points.end(); ++i)
	{
		renderResponseXml(element.get(), *i);
	}

	// Release ownership to avoid deletion
	element.release();  // NOLINT (must release and disregard the returned pointer)
}

/// Render a InterpPoint as CSV
void Response::renderResponseCsv(std::ofstream& of, const InterpPoint& point) const
{
	of << point.time << ", " << point.power << ", " << point.phase << ", " << _wave->getCarrier() * (1 - point.doppler)
		<< "\n";
}

/// Render the response to a CSV file
void Response::renderCsv(std::ofstream& of)
{
	//Render each interpolation point
	for (std::vector<InterpPoint>::const_iterator i = _points.begin(); i != _points.end(); ++i)
	{
		renderResponseCsv(of, *i);
	}
}

/// Add an interp point to the vector
void Response::addInterpPoint(const InterpPoint& point)
{
	// Check that points are being added in order
	if ((!_points.empty()) && (point.time < _points.back().time))
	{
		throw std::logic_error("[BUG] Interpolation points not being added in order");
	}
	// This method does not need a mutex as only one thread owns any non-const Response object
	_points.push_back(point);
}

/// Render the response to an array
boost::shared_array<RsComplex> Response::renderBinary(rsFloat& rate, unsigned int& size,
                                                      const rsFloat fracWinDelay) const
{
	rate = _wave->getRate();
	return _wave->render(_points, size, fracWinDelay);
}
