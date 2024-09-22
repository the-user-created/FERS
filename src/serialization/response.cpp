// response.cpp
// Implementation of ResponseBase and derived classes
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 3 August 2006

#define TIXML_USE_STL

#include "response.h"

#include <iomanip>
#include <memory>
#include <tinyxml.h>

#include "radar/radar_system.h"
#include "signal_processing/radar_signal.h"

using interp::InterpPoint;

namespace
{
	void attachTextNode(TiXmlElement* root, const std::string& name, const std::string& text)
	{
		auto element = std::make_unique<TiXmlElement>(name);
		auto xtext = std::make_unique<TiXmlText>(text);
		element->LinkEndChild(xtext.release());
		root->LinkEndChild(element.release());
	}

	void attachRsFloatNode(TiXmlElement* root, const std::string& name, const RealType data,
	                       const bool scientific = true)
	{
		std::ostringstream oss;
		if (scientific) { oss.setf(std::ios::scientific); }
		constexpr int precision = 10;
		oss << std::setprecision(precision) << data;
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

	void Response::renderResponseXml(TiXmlElement* root, const InterpPoint& point) const
	{
		auto* element = new TiXmlElement("InterpolationPoint");
		root->LinkEndChild(element);

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

	void Response::renderXml(TiXmlElement* root)
	{
		auto* element = new TiXmlElement("Response");
		root->LinkEndChild(element);

		element->SetAttribute("transmitter", getTransmitterName());
		attachRsFloatNode(element, "start", startTime(), false);
		attachTextNode(element, "name", _wave->getName());

		for (const auto& point : _points) { renderResponseXml(element, point); }
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
