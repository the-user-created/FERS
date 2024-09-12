// signal.cpp
// Interface for the signal class
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started 24 May 2006

#ifndef RS_SIGNAL_H
#define RS_SIGNAL_H

#include "config.h"
#include "radar_signal.h"

// TODO: this dual namespace is funky
// TODO: move to radar_signal.h - only used there

namespace signal
{
	void addNoise(RS_FLOAT* data, RS_FLOAT temperature, unsigned size, RS_FLOAT fs); // TODO: unused function

	RS_COMPLEX* iqDemodulate(RS_FLOAT* data, unsigned size, RS_FLOAT phase); // TODO: unused function

	// TODO: only used in rspulserender.cpp
	void adcSimulate(RS_COMPLEX* data, unsigned size, unsigned bits, RS_FLOAT fullscale);
}

namespace rs
{
	struct InterpPoint;

	class Signal
	{
	public:
		Signal() : _data(nullptr), _size(0), _rate(0)
		{
		}

		~Signal()
		{
			delete[] _data;
		}

		void clear();

		void load(const RS_COMPLEX* inData, unsigned samples, RS_FLOAT sampleRate);

		void load(const RS_FLOAT* inData, unsigned samples, RS_FLOAT sampleRate);

		[[nodiscard]] RS_FLOAT rate() const
		{
			return _rate;
		}

		[[nodiscard]] unsigned size() const
		{
			return _size;
		}

		[[nodiscard]] RS_FLOAT* copyData() const;

		boost::shared_array<RS_COMPLEX> render(const std::vector<InterpPoint>& points,
		                                       unsigned& size, double fracWinDelay) const;

	private:
		RS_COMPLEX* _data;
		unsigned _size;
		RS_FLOAT _rate;
	};
}

#endif
