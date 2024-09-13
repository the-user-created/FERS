// response.h
// Classes for responses created by simulation
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 3 August 2006

#ifndef RESPONSE_H
#define RESPONSE_H

#include "config.h"
#include "radar/radar_signal.h"

class TiXmlElement;

namespace rs
{
	class Antenna;
	class Transmitter;

	class Response : boost::noncopyable
	{
	public:
		Response(const RadarSignal* wave, const Transmitter* transmitter) :
			_transmitter(transmitter), _wave(wave) {}

		~Response() = default;

		[[nodiscard]] RS_FLOAT startTime() const { return _points.empty() ? 0.0 : _points.front().time; }

		[[nodiscard]] RS_FLOAT endTime() const { return _points.empty() ? 0.0 : _points.back().time; }

		void renderXml(TiXmlElement* root);

		void renderCsv(std::ofstream& of);

		std::shared_ptr<RS_COMPLEX[]> renderBinary(RS_FLOAT& rate, unsigned& size, RS_FLOAT fracWinDelay) const;

		[[nodiscard]] RS_FLOAT getLength() const { return endTime() - startTime(); }

		[[nodiscard]] const RadarSignal* getWave() const { return _wave; }

		[[nodiscard]] std::string getTransmitterName() const;

		void addInterpPoint(const InterpPoint& point);

	protected:
		const Transmitter* _transmitter;

		void renderResponseXml(TiXmlElement* root, const InterpPoint& point) const;

		void renderResponseCsv(std::ofstream& of, const InterpPoint& point) const;

		const RadarSignal* _wave;
		std::vector<InterpPoint> _points;
	};
}

#endif
