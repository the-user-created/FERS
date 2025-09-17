// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
* @file response.cpp
* @brief Implementation of the Response class
*/

#include "response.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "libxml_wrapper.h"
#include "radar/radar_obj.h"
#include "radar/transmitter.h"
#include "signal/radar_signal.h"

using interp::InterpPoint;

namespace
{
	/**
	 * @brief Attach a text node to an XML element.
	 *
	 * @param root The parent XML element to attach the text node to.
	 * @param name The name of the child element to create.
	 * @param text The text content to set for the child element.
	 */
	void attachTextNode(const XmlElement& root, const std::string& name, const std::string& text) noexcept
	{
		const XmlElement element = root.addChild(name);

		element.setText(text);
	}

	/**
	 * @brief Attach a float node to an XML element.
	 *
	 * @param root The parent XML element to attach the float node to.
	 * @param name The name of the child element to create.
	 * @param data The float value to format and set as text content.
	 * @param scientific Whether to use scientific notation for the float value.
	 */
	void attachRsFloatNode(const XmlElement& root, const std::string& name, const RealType data,
	                       const bool scientific = true) noexcept
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
	std::string Response::getTransmitterName() const noexcept { return _transmitter->getName(); }

	void Response::renderResponseXml(const XmlElement& root, const InterpPoint& point) const noexcept
	{
		const XmlElement element = root.addChild("InterpolationPoint");

		attachRsFloatNode(element, "time", point.time, false);
		attachRsFloatNode(element, "amplitude", std::sqrt(point.power * _wave->getPower()), false);
		attachRsFloatNode(element, "phase", point.phase, false);
		attachRsFloatNode(element, "doppler", _wave->getCarrier() * (point.doppler_factor - 1.0), false);
		attachRsFloatNode(element, "power", point.power * _wave->getPower());
		attachRsFloatNode(element, "Iamplitude",
		                  std::cos(point.phase) * std::sqrt(point.power * _wave->getPower()));
		attachRsFloatNode(element, "Qamplitude",
		                  std::sin(point.phase) * std::sqrt(point.power * _wave->getPower()));
		attachRsFloatNode(element, "noise_temperature", point.noise_temperature);
		attachRsFloatNode(element, "phasedeg", point.phase / PI * 180);
	}

	void Response::renderXml(const XmlElement& root) const noexcept
	{
		const XmlElement element = root.addChild("Response");

		element.setAttribute("transmitter", getTransmitterName());

		attachRsFloatNode(element, "start", startTime(), false);
		attachTextNode(element, "name", _wave->getName());

		for (const auto& point : _points) { renderResponseXml(element, point); }
	}

	void Response::renderCsv(std::ofstream& of) const noexcept
	{
		for (const auto& point : _points)
		{
			of << point.time << ", " << point.power << ", " << point.phase << ", " << _wave->getCarrier() * (point.
				doppler_factor - 1.0) << "\n";
		}
	}

	void Response::addInterpPoint(const InterpPoint& point) { _points.push_back(point); }

	std::vector<ComplexType> Response::renderBinary(RealType& rate, unsigned& size, const RealType fracWinDelay) const
	{
		rate = _wave->getRate();
		return _wave->render(_points, size, fracWinDelay);
	}
}
