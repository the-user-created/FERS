//rsresponse.h
// Classes for responses created by simulation
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 3 August 2006

#ifndef RS_RESPONSE_H
#define RS_RESPONSE_H

#include <iosfwd>
#include <string>
#include <vector>
#include <boost/shared_array.hpp>
#include <boost/utility.hpp>

#include "config.h"
#include "rsradarwaveform.h"

//Forward declaration of TiXmlElement (see tinyxml.h)
class TiXmlElement;

namespace rs
{
	//Forward definition of Antenna (see rsantenna.h)
	class Antenna;
	//Forward definition of Transmitter (in this file)
	class Transmitter;

	/// Class for both pulsed and CW responses
	class Response : boost::noncopyable
	{
	public:
		/// Constructor
		Response(const RadarSignal* wave, const Transmitter* transmitter);

		/// Get the start time
		[[nodiscard]] RS_FLOAT startTime() const;

		[[nodiscard]] RS_FLOAT endTime() const;

		/// Destructor
		~Response();

		/// Render the response to an XML file
		void renderXml(TiXmlElement* root);

		/// Render the response to a CSV file
		void renderCsv(std::ofstream& of);

		/// Render the response to a binary file
		boost::shared_array<RsComplex> renderBinary(RS_FLOAT& rate, unsigned int& size, RS_FLOAT fracWinDelay) const;

		/// Get the length of the pulse
		[[nodiscard]] RS_FLOAT getLength() const;

		/// Get a pointer to the wave
		[[nodiscard]] const RadarSignal* getWave() const;

		/// Get the name of the transmitter that started this response
		[[nodiscard]] std::string getTransmitterName() const;

		/// Add an interp point to the vector
		void addInterpPoint(const InterpPoint& point);

	protected:
		const Transmitter* _transmitter; //!< The transmitter that caused this response
		void renderResponseXml(TiXmlElement* root, const InterpPoint& point) const;

		void renderResponseCsv(std::ofstream& of, const InterpPoint& point) const; //!< Render a InterpPoint as CSV
		const RadarSignal* _wave; //!< The waveform sent out by the transmitter
		std::vector<InterpPoint> _points; //!< Waypoints from which the response parameters are interpolated
	};
}

#endif
