// response.h
// Classes for responses created by simulation
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 3 August 2006

#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "config.h"
#include "interpolation/interpolation_point.h"

class XmlElement;

namespace signal
{
	class RadarSignal;
}

namespace radar
{
	class Transmitter;
}

namespace serial
{
	class Response
	{
	public:
		Response(const signal::RadarSignal* wave, const radar::Transmitter* transmitter) noexcept :
			_transmitter(transmitter), _wave(wave) {}

		~Response() = default;

		// Delete copy constructor and copy assignment operator to prevent copying
		Response(const Response&) = delete;

		Response& operator=(const Response&) = delete;

		[[nodiscard]] RealType startTime() const noexcept { return _points.empty() ? 0.0 : _points.front().time; }
		[[nodiscard]] RealType endTime() const noexcept { return _points.empty() ? 0.0 : _points.back().time; }

		void addInterpPoint(const interp::InterpPoint& point);

		void renderXml(const XmlElement& root) noexcept;

		void renderCsv(std::ofstream& of) const noexcept;

		std::vector<ComplexType> renderBinary(RealType& rate, unsigned& size, RealType fracWinDelay) const;

		[[nodiscard]] RealType getLength() const noexcept { return endTime() - startTime(); }

		[[nodiscard]] std::string getTransmitterName() const noexcept;

	private:
		const radar::Transmitter* _transmitter;
		const signal::RadarSignal* _wave;
		std::vector<interp::InterpPoint> _points;

		void renderResponseXml(const XmlElement& root, const interp::InterpPoint& point) const noexcept;

		void renderResponseCsv(std::ofstream& of, const interp::InterpPoint& point) const noexcept;
	};
}
