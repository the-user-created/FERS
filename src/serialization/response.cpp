// response.cpp
// Implementation of ResponseBase and derived classes
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 3 August 2006

#define TIXML_USE_STL

#include "response.h"

#include <cmath>                             // for sqrt, cos, sin
#include <iomanip>                           // for operator<<, setprecision
#include <sstream>                           // for basic_ostringstream
#include <stdexcept>                         // for logic_error

#include "libxml_wrapper.h"
#include "radar/radar_system.h"              // for Transmitter
#include "signal_processing/radar_signal.h"  // for RadarSignal

using interp::InterpPoint;

namespace
{
	void attachTextNode(XmlElement& root, const std::string& name, const std::string& text)
	{
		// Create a child element with the given name
		XmlElement element = root.addChild(name);

		// Set the text content of the element
		element.setText(text);
	}

	void attachRsFloatNode(XmlElement& root, const std::string& name, const RealType data, const bool scientific = true)
	{
		std::ostringstream oss;
		if (scientific) {
			oss.setf(std::ios::scientific);
		}
		constexpr int precision = 10;
		oss << std::setprecision(precision) << data;

		// Attach the text node with the formatted float value
		attachTextNode(root, name, oss.str());
	}
}

namespace serial
{
	// =================================================================================================================
	//
	// RESPONSE CLASS
	//
	// =================================================================================================================

	std::string Response::getTransmitterName() const { return _transmitter->getName(); }

	void Response::renderResponseXml(XmlElement& root, const InterpPoint& point) const
	{
		// Create an "InterpolationPoint" element and link it to the root
		XmlElement element = root.addChild("InterpolationPoint");

		// Attach the various float nodes, using point data
		attachRsFloatNode(element, "time", point.time, false);
		attachRsFloatNode(element, "amplitude", std::sqrt(point.power * _wave->getPower()), false);
		attachRsFloatNode(element, "phase", point.phase, false);
		attachRsFloatNode(element, "doppler", _wave->getCarrier() * (1 - point.doppler), false);
		attachRsFloatNode(element, "power", point.power * _wave->getPower());
		attachRsFloatNode(element, "Iamplitude",
						  std::cos(point.phase) * std::sqrt(point.power * _wave->getPower()));
		attachRsFloatNode(element, "Qamplitude",
						  std::sin(point.phase) * std::sqrt(point.power * _wave->getPower()));
		attachRsFloatNode(element, "noise_temperature", point.noise_temperature);
		attachRsFloatNode(element, "phasedeg", point.phase / PI * 180);
	}

	void Response::renderXml(XmlElement& root)
	{
		// Create a "Response" element and link it to the root
		XmlElement element = root.addChild("Response");

		// Set the "transmitter" attribute
		element.setAttribute("transmitter", getTransmitterName());

		// Attach various properties
		attachRsFloatNode(element, "start", startTime(), false);
		attachTextNode(element, "name", _wave->getName());

		// Iterate over points and render their XML representation
		for (const auto& point : _points) {
			renderResponseXml(element, point);
		}
	}

	void Response::renderResponseCsv(std::ofstream& of, const InterpPoint& point) const
	{
		of << point.time << ", " << point.power << ", " << point.phase << ", "
			<< _wave->getCarrier() * (1 - point.doppler) << "\n";
	}

	void Response::renderCsv(std::ofstream& of) const
	{
		for (const auto& point : _points) { renderResponseCsv(of, point); }
	}

	void Response::addInterpPoint(const InterpPoint& point)
	{
		if (!_points.empty() && point.time < _points.back().time)
		{
			throw std::logic_error("[BUG] Interpolation points not being added in order");
		}
		_points.push_back(point);
	}

	std::vector<ComplexType> Response::renderBinary(RealType& rate, unsigned& size, const RealType fracWinDelay) const
	{
		rate = _wave->getRate();
		return _wave->render(_points, size, fracWinDelay);
	}
}
