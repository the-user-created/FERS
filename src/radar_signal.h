// radar_signal.h
// Classes for different types of radar waveforms
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 8 June 2006

#ifndef RADAR_SIGNAL_H
#define RADAR_SIGNAL_H

#include <memory>
#include <vector>
#include <boost/utility.hpp>

#include "config.h"
#include "interpolation_point.h"
#include "jones_vector.h"
#include "rssignal.h"

namespace rs
{
	class Signal; // Forward declaration

	class RadarSignal : public boost::noncopyable
	{
	public:
		RadarSignal(std::string name, RS_FLOAT power, RS_FLOAT carrierfreq, RS_FLOAT length, Signal* signal);

		~RadarSignal();

		[[nodiscard]] RS_FLOAT getPower() const { return _power; }

		[[nodiscard]] RS_FLOAT getCarrier() const;

		[[nodiscard]] std::string getName() const { return _name; }

		[[nodiscard]] RS_FLOAT getRate() const;

		[[nodiscard]] RS_FLOAT getLength() const { return _length; }

		std::shared_ptr<RS_COMPLEX[]> render(const std::vector<InterpPoint>& points, unsigned& size,
		                                     RS_FLOAT fracWinDelay) const;

		[[nodiscard]] JonesVector getPolarization() const { return _polar; }

		void setPolarization(const JonesVector& in) { _polar = in; }

	private:
		std::string _name;
		RS_FLOAT _power;
		RS_FLOAT _carrierfreq;
		RS_FLOAT _length;
		Signal* _signal;
		JonesVector _polar;
	};
}

namespace pulse_factory
{
	rs::RadarSignal* loadPulseFromFile(const std::string& name, const std::string& filename, RS_FLOAT power,
	                                   RS_FLOAT carrierFreq);
}


#endif
