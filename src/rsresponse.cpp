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
		auto element = std::make_unique<TiXmlElement>(name);
		auto xtext = std::make_unique<TiXmlText>(text);
		element->LinkEndChild(xtext.release());
		root->LinkEndChild(element.release());
	}

	/// Attach a text node to an XML element, getting the text by converting a rsFloat to a string
	// For creating structures like this
	//<node>
	//<name>text</name>
	//</node>
	void attachRsFloatNode(TiXmlElement* root, const std::string& name, const RS_FLOAT data,
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
}

//
// Interppoint Implementation
//
/// InterpPoint Constructor
InterpPoint::InterpPoint(const RS_FLOAT power, const RS_FLOAT start, const RS_FLOAT delay, const RS_FLOAT doppler,
                         const RS_FLOAT phase, const RS_FLOAT noiseTemperature):
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
RS_FLOAT Response::startTime() const
{
	if (_points.empty())
	{
		return 0;
	}
	return _points.front().time;
}

/// Return the time the pulse's energy ends
RS_FLOAT Response::endTime() const
{
	if (_points.empty())
	{
		return 0;
	}
	return _points.back().time;
}

/// Return the length of the pulse
RS_FLOAT Response::getLength() const
{
	return endTime() - startTime();
}

//Get the name of the transmitter which caused this response
std::string Response::getTransmitterName() const
{
	return _transmitter->getName();
}

/// Return a pointer to the waveform
const RadarSignal* Response::getWave() const
{
	return _wave;
}

/// Render a single response point to XML
void Response::renderResponseXml(TiXmlElement* root, const InterpPoint& point) const
{
	// Create a node for the response
	auto element = std::make_unique<TiXmlElement>("InterpolationPoint");
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
	auto element = std::make_unique<TiXmlElement>("Response");
	root->LinkEndChild(element.get());
	element->SetAttribute("transmitter", getTransmitterName());

	// Attach nodes for properties of the response
	attachRsFloatNode(element.get(), "start", startTime(), false);
	attachTextNode(element.get(), "name", _wave->getName());

	// Render each interpolation point in turn
	for (auto i = _points.begin(); i != _points.end(); ++i)
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
	for (auto i = _points.begin(); i != _points.end(); ++i)
	{
		renderResponseCsv(of, *i);
	}
}

/// Add an interp point to the vector
void Response::addInterpPoint(const InterpPoint& point)
{
	// Check that points are being added in order
	if (!_points.empty() && point.time < _points.back().time)
	{
		throw std::logic_error("[BUG] Interpolation points not being added in order");
	}
	// This method does not need a mutex as only one thread owns any non-const Response object
	_points.push_back(point);
}

/// Render the response to an array
boost::shared_array<RsComplex> Response::renderBinary(RS_FLOAT& rate, unsigned int& size,
                                                      const RS_FLOAT fracWinDelay) const
{
	rate = _wave->getRate();
	return _wave->render(_points, size, fracWinDelay);
}
