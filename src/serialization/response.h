// response.h
// Classes for responses created by simulation
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 3 August 2006

#ifndef RESPONSE_H
#define RESPONSE_H

#include <fstream>                              // for ofstream
#include <string>                               // for string
#include <vector>                               // for vector

#include "config.h"                             // for RealType, ComplexType
#include "interpolation/interpolation_point.h"  // for InterpPoint

class TiXmlElement;

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
		Response(const signal::RadarSignal* wave, const radar::Transmitter* transmitter) : _transmitter(transmitter),
			_wave(wave) {}

		~Response() = default;

		// Delete copy constructor and copy assignment operator to prevent copying
		Response(const Response&) = delete;

		Response& operator=(const Response&) = delete;

		[[nodiscard]] RealType startTime() const { return _points.empty() ? 0.0 : _points.front().time; }
		[[nodiscard]] RealType endTime() const { return _points.empty() ? 0.0 : _points.back().time; }

		void addInterpPoint(const interp::InterpPoint& point);

		void renderXml(TiXmlElement* root);

		void renderCsv(std::ofstream& of) const;

		std::vector<ComplexType> renderBinary(RealType& rate, unsigned& size, RealType fracWinDelay) const;

		[[nodiscard]] RealType getLength() const { return endTime() - startTime(); }
		[[nodiscard]] const signal::RadarSignal* getWave() const { return _wave; } // Note: This function is not used in the codebase

		[[nodiscard]] std::string getTransmitterName() const;

	private:
		const radar::Transmitter* _transmitter;
		const signal::RadarSignal* _wave;
		std::vector<interp::InterpPoint> _points;

		void renderResponseXml(TiXmlElement* root, const interp::InterpPoint& point) const;

		void renderResponseCsv(std::ofstream& of, const interp::InterpPoint& point) const;
	};
}

#endif
