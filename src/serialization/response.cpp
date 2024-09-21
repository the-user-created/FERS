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

	void attachRsFloatNode(TiXmlElement* root, const std::string& name, const RS_FLOAT data,
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
		auto element = std::make_unique<TiXmlElement>("InterpolationPoint");
		root->LinkEndChild(element.get());

		attachRsFloatNode(element.get(), "time", point.time, false);
		attachRsFloatNode(element.get(), "amplitude", std::sqrt(point.power * _wave->getPower()), false);
		attachRsFloatNode(element.get(), "phase", point.phase, false);
		attachRsFloatNode(element.get(), "doppler", _wave->getCarrier() * (1 - point.doppler), false);
		attachRsFloatNode(element.get(), "power", point.power * _wave->getPower());
		attachRsFloatNode(element.get(), "Iamplitude",
		                  std::cos(point.phase) * std::sqrt(point.power * _wave->getPower()));
		attachRsFloatNode(element.get(), "Qamplitude",
		                  std::sin(point.phase) * std::sqrt(point.power * _wave->getPower()));
		attachRsFloatNode(element.get(), "noise_temperature", point.noise_temperature);
		attachRsFloatNode(element.get(), "phasedeg", point.phase / M_PI * 180);

		element.release(); // TODO: this is a bad practice
	}

	void Response::renderXml(TiXmlElement* root)
	{
		auto element = std::make_unique<TiXmlElement>("Response");
		root->LinkEndChild(element.get());

		element->SetAttribute("transmitter", getTransmitterName());
		attachRsFloatNode(element.get(), "start", startTime(), false);
		attachTextNode(element.get(), "name", _wave->getName());

		for (const auto& point : _points) { renderResponseXml(element.get(), point); }

		element.release(); // TODO: this is a bad practice
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

	std::vector<RS_COMPLEX> Response::renderBinary(RS_FLOAT& rate, unsigned& size, const RS_FLOAT fracWinDelay) const
	{
		rate = _wave->getRate();
		return _wave->render(_points, size, fracWinDelay);
	}
}
