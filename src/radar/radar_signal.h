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
#include "jones_vector.h"
#include "interpolation/interpolation_point.h"

namespace rs
{
	class Signal
	{
	public:
		Signal() : _data(nullptr), _size(0), _rate(0) {}

		~Signal() { delete[] _data; }

		void clear();

		void load(const RS_COMPLEX* inData, unsigned samples, RS_FLOAT sampleRate);

		void load(const RS_FLOAT* inData, unsigned samples, RS_FLOAT sampleRate);

		[[nodiscard]] RS_FLOAT getRate() const { return _rate; }

		[[nodiscard]] unsigned getSize() const { return _size; }

		// Note: This function is not used in the codebase
		[[nodiscard]] RS_FLOAT* copyData() const;

		std::shared_ptr<std::complex<double>[]> render(const std::vector<InterpPoint>& points, unsigned& size,
		                                               double fracWinDelay) const;

	private:
		RS_COMPLEX* _data;
		unsigned _size;
		RS_FLOAT _rate;

		[[nodiscard]] std::tuple<double, double, double, int> calculateWeightsAndDelays(
			std::vector<InterpPoint>::const_iterator iter, std::vector<InterpPoint>::const_iterator next,
			double sampleTime, double idelay, double fracWinDelay) const;

		std::complex<double> performConvolution(int i, const double* filt, int filtLength, double amplitude,
		                                        int iSampleUnwrap) const;
	};

	class RadarSignal : public boost::noncopyable
	{
	public:
		RadarSignal(std::string name, RS_FLOAT power, RS_FLOAT carrierfreq, RS_FLOAT length, Signal* signal);

		~RadarSignal() { delete _signal; }

		[[nodiscard]] RS_FLOAT getPower() const { return _power; }

		[[nodiscard]] RS_FLOAT getCarrier() const;

		[[nodiscard]] std::string getName() const { return _name; }

		[[nodiscard]] RS_FLOAT getRate() const { return _signal->getRate(); }

		[[nodiscard]] RS_FLOAT getLength() const { return _length; }

		std::shared_ptr<RS_COMPLEX[]> render(const std::vector<InterpPoint>& points, unsigned& size,
		                                     RS_FLOAT fracWinDelay) const;

		// Note: This function is not used in the codebase
		[[nodiscard]] JonesVector getPolarization() const { return _polar; }

		// Note: This function is not used in the codebase
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

#endif
