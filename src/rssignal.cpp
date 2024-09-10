//rssignal.cpp - Class to contain an arbitrary frequency domain signal
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//24 May 2006

#include "rssignal.h"

#include <cmath>
#include <stdexcept>
#include <boost/thread/mutex.hpp>

#include "rsdebug.h"
#include "rsdsp.h"
#include "rsparameters.h"
#include "rsportable.h"

using namespace rs_signal;
using namespace rs;
//Global mutex to protect the InterpFilter filter calculation function
boost::mutex interp_mutex;

namespace
{
	/// Compute the zeroth order modified bessel function of the first kind
	// Use the polynomial approximation from section 9.8 of
	// "Handbook of Mathematical Functions" by Abramowitz and Stegun
	rsFloat
	besselI0(const rsFloat x)
	{
		rsFloat i0;
		if (rsFloat t = (x / 3.75); t < 0.0)
		{
			throw std::logic_error("Modified Bessel approximation only valid for x > 0");
		}
		else if (t <= 1.0)
		{
			t *= t;
			i0 = 1.0 + t * (3.5156229 + t * (3.0899424 + t * (1.2067492 + t * (0.2659732 + t * (0.0360768 + t *
				0.0045813)))));
			//Error bounded to 1.6e-7
		}
		else
		{
			//t > 1;
			i0 = 0.39894228
				+ t
				* (0.01328592
					+ t * (0.00225319 + t * (-0.00157565 + t * (0.00916281 + t * (-0.02057706 + t * (0.02635537 + t * (-
						0.01647633 + t * 0.00392377)))))));
			i0 *= std::exp(x) / std::sqrt(x);
			//Error bounded to 1.9e-7
		}
		return i0;
	}

	class InterpFilter
	{
	public:
		/// Compute the sinc function at the specified x
		static inline rsFloat sinc(rsFloat x);

		/// Compute the value of a Kaiser Window at the given x
		inline rsFloat kaiserWinCompute(rsFloat x) const;

		/// Calculate the value of the interpolation filter at time x
		inline rsFloat interpFilter(rsFloat x) const;

		/// Get a pointer to the filter with approximately the specified delay
		const rsFloat* getFilter(rsFloat delay) const;

		/// Get a pointer to the class instance
		static InterpFilter* getInstance()
		{
			// Protect this with a mutex --- all other operations are const
			boost::mutex::scoped_lock lock(interp_mutex);
			if (!_instance)
			{
				_instance = new InterpFilter();
			}
			return _instance;
		}

	private:
		//// Default constructor
		InterpFilter();

		/// Pointer to a single instance of the class
		static InterpFilter* _instance;

		rsFloat _alpha; //!< 'alpha' parameter
		rsFloat _beta; //!< 'beta' parameter
		rsFloat _bessel_beta; //!< I0(beta)
		int _length;
		int _table_filters; //!< Number of filters in the filter table
		rsFloat* _filter_table; //!< Table of precalculated filters
	};

	/// Interpfilter class constructor
	InterpFilter::InterpFilter()
	{
		_length = RsParameters::renderFilterLength();
		//Size of the table to use for interpolation
		_table_filters = 1000;
		//Allocate memory for the table
		_filter_table = new rsFloat[_table_filters * _length];
		//Alpha is half the filter length
		_alpha = std::floor(RsParameters::renderFilterLength() / 2.0);
		//Beta sets the window shape
		_beta = 5;
		_bessel_beta = besselI0(_beta);
		const int hfilt = _table_filters / 2;
		rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Building table of %d filters\n", _table_filters);
		//Fill the table of filters
		//C Tong: delay appears to be the fraction of time ellapsed between samples
		for (int i = -hfilt; i < hfilt; i++)
		{
			const rsFloat delay = i / static_cast<rsFloat>(hfilt);
			for (int j = -_alpha; j < _alpha; j++)
			{
				_filter_table[static_cast<int>((i + hfilt) * _length + j + _alpha)] = interpFilter(j - delay);
			}
		}
		rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Filter table complete.\n");
	}

	/// Get a pointer to the filter with approximately the specified delay
	const rsFloat*
	InterpFilter::getFilter(const rsFloat delay) const
	{
		const int filt = (delay + 1) * (_table_filters / 2);

		if ((delay <= -1) || (delay >= 1))
		{
			rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "GetFilter %f %d\n", delay, filt);
			throw std::runtime_error("[BUG] Requested delay filter value out of range");
		}

		//rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "GetFilter %f %d\n", delay, filt);
		return &(_filter_table[filt * _length]);
	}

	/// Lookup the value of the interpolation filter at time x
	rsFloat InterpFilter::interpFilter(const rsFloat x) const
	{
		const rsFloat w = kaiserWinCompute(x + _alpha);
		const rsFloat s = sinc(x); //The filter value
		const rsFloat filt = w * s;
		//      rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "%g %g\n", t, filt);
		return filt;
	}

	/// Compute the sinc function at the specified x
	rsFloat InterpFilter::sinc(const rsFloat x)
	{
		if (x == 0)
		{
			return 1.0;
		}
		return std::sin(x * M_PI) / (x * M_PI);
	}

	/// Compute the value of a Kaiser Window at the given x
	rsFloat
	InterpFilter::kaiserWinCompute(const rsFloat x) const
	{
		if ((x < 0) || (x > (_alpha * 2)))
		{
			return 0;
		}
		else
		{
			return besselI0(_beta * std::sqrt(1 - std::pow((x - _alpha) / _alpha, 2))) / _bessel_beta;
		}
	}

	//Create an instance of InterpFilter
	InterpFilter* InterpFilter::_instance = nullptr;
}

/// Simulate the effect of and ADC converter on the signal
void rs_signal::adcSimulate(Complex* data, const unsigned int size, const int bits, const rsFloat fullscale)
{
	//Get the number of levels associated with the number of bits
	const rsFloat levels = pow(2, bits - 1);
	for (unsigned int it = 0; it < size; it++)
	{
		//Simulate the ADC effect on the I and Q samples
		rsFloat i = std::floor(levels * data[it].real() / fullscale) / levels;
		rsFloat q = std::floor(levels * data[it].imag() / fullscale) / levels;
		//Clamp I and Q to the range, simulating saturation in the adc
		if (i > 1)
		{
			i = 1;
		}
		else if (i < -1)
		{
			i = -1;
		}
		if (q > 1)
		{
			q = 1;
		}
		else if (q < -1)
		{
			q = -1;
		}
		//Overwrite data with the results
		data[it] = Complex(i, q);
	}
}

//
// Signal Implementation
//

//Default constructor for brute signal
Signal::Signal() :
	_data(nullptr), _size(0), _rate(0)
{
}

//Default destructor for brutesignal
Signal::~Signal()
{
	delete[] _data;
}

//Clear the data array, emptying the signal and freeing memory
void
Signal::clear()
{
	delete[] _data;
	_size = 0;
	_rate = 0;
	_data = nullptr; //Set data to zero to prevent multiple frees
}

//Load data into the signal, with the given sample rate and size
void Signal::load(const rsFloat* inData, const unsigned int samples, const rsFloat sampleRate)
{
	//Remove the previous data
	clear();
	//Set the size and samples attributes
	_size = samples;
	_rate = sampleRate;
	//Allocate memory for the signal
	_data = new Complex[samples];
	//Copy the data
	for (unsigned int i = 0; i < samples; i++)
	{
		_data[i] = Complex(inData[i], 0.0);
	}
}

/// Load data into the signal (time domain, complex)
void Signal::load(const Complex* inData, const unsigned int samples, const rsFloat sampleRate)
{
	//Remove the previous data
	clear();
	// Get the oversampling ratio
	const unsigned int ratio = RsParameters::oversampleRatio();
	//Allocate memory for the signal
	_data = new Complex[samples * ratio];
	//Set the size and samples attributes
	_size = samples * ratio;
	_rate = sampleRate * ratio;
	if (ratio == 1)
	{
		//Copy the data (using a loop for now, not sure memcpy() will always work on complex)
		for (unsigned int i = 0; i < samples; i++)
		{
			_data[i] = inData[i];
		}
	}
	else
	{
		// Upsample the data into the target buffer
		upsample(inData, samples, _data, ratio);
	}
}

//Return the sample rate of the signal
rsFloat Signal::rate() const
{
	return _rate;
}

//Return the size, in samples, of the signal
unsigned int Signal::size() const
{
	return _size;
}

/// Get a copy of the signal domain data
rsFloat* Signal::copyData() const
{
	rsFloat* result = new rsFloat[_size];
	//Copy the data into result
	std::memcpy(result, _data, sizeof(rsFloat) * _size);
	return result;
}

/// Get the number of samples of padding at the beginning of the pulse
unsigned int Signal::pad() const
{
	return _pad;
}

/// Render the pulse with the specified doppler, delay and amplitude
boost::shared_array<RsComplex> Signal::render(const std::vector<InterpPoint>& points, rsFloat transPower,
                                              unsigned int& size, const rsFloat fracWinDelay) const
{
	//Allocate memory for rendering
	RsComplex* out = new RsComplex[_size];
	size = _size;

	//Get the sample interval
	const rsFloat timestep = 1.0 / _rate;
	//Create the rendering window
	const int filt_length = RsParameters::renderFilterLength();
	const InterpFilter* interp = InterpFilter::getInstance();
	//Loop through the interp points, rendering each in time
	std::vector<rs::InterpPoint>::const_iterator iter = points.begin();
	std::vector<rs::InterpPoint>::const_iterator next = iter + 1;
	if (next == points.end())
	{
		next = iter;
	}

	//Get the delay of the first point
	//C Tong: iDelay is in number of receiver samples (possibly with a fractional part)
	const rsFloat idelay = rs_portable::rsRound(_rate * (*iter).delay);
	//rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "idelay = %g\n", idelay);

	//Memory to store the filter in

	//Loop over the pulse, performing the rendering
	rsFloat sample_time = (*iter).time;
	for (int i = 0; i < static_cast<int>(_size); i++, sample_time += timestep)
	{
		//Check if we should move on to the next set of interp points
		if ((sample_time > (*next).time))
		{
			iter = next;
			if ((next + 1) != points.end())
			{
				++next;
			}
		}
		//Get the weightings for the parameters
		rsFloat aw = 1, bw = 0;
		if (iter < next)
		{
			bw = (sample_time - (*iter).time) / ((*next).time - (*iter).time);
			aw = 1 - bw;

			//**C Tong:**
			//bw is fraction of time elapsed between 2 samples (this is interpolated)
			//aw is then the fraction of time remaining
		}

		//Now calculate the current sample parameters
		rsFloat amplitude = std::sqrt((*iter).power) * aw + std::sqrt((*next).power) * bw;
		rsFloat phase = (*iter).phase * aw + (*next).phase * bw;
		rsFloat fdelay = -(((*iter).delay * aw + (*next).delay * bw) * _rate - idelay + fracWinDelay);

		//if ((*iter).delay * 299792458 != 11000)
		//{
		//rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "%g\n", frac_win_delay);
		//rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "time = %gs, fdelay = %g, range = : %gm\n", (*iter).time, fdelay, (*iter).delay * 299792458);
		//rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Amp: %g Delay: %g + %g Phase: %g\n", amplitude, (*iter).delay, fdelay, phase);
		//}

		//C Tong: Fixing the |fdelay| > 1 problem.
		//Tx samples and Rx samples are matched 1 to 1 in time pulse the addition of
		//the fraction delay filters to create the delay effect.
		//For long pulses or CW when in there are fast moving targets present
		//the fractional delay can go 1 whole sample which will crash the program.

		//As a fix this we need to look for this "wrapping" effect and
		//unwrap it by using the previous or next sample from the Tx signal.
		//This is a hack at best and the program should really be re-designed.

		const int i_sample_unwrap = floor(fdelay); //Number of samples to unwrap by.
		fdelay -= i_sample_unwrap; //Re-calculate the delay filter for the given delay

		const rsFloat* filt = interp->getFilter(fdelay);

		//Get the start and end times of interpolation
		int start = -filt_length / 2;
		if ((i + start) < 0)
		{
			start = -i;
		}

		int end = filt_length / 2;
		if ((i + end) >= _size)
		{
			end = _size - i;
		}

		//Apply the filter
		Complex accum(0.0, 0.0);

		for (int j = start; j < end; j++)
		{
			//Check that unwrapping doesn't put us out of bounds.
			if (i + j + i_sample_unwrap >= _size || i + j + i_sample_unwrap < 0)
			{
				continue;
			}

			accum += amplitude * _data[i + j + i_sample_unwrap] * filt[j + filt_length / 2];
			//Apply unwrapping to Tx samples.
			if (std::isnan(_data[j].real()))
			{
				throw std::runtime_error("NAN in Render: data[j].r");
			}
			if (std::isnan(_data[j].imag()))
			{
				throw std::runtime_error("NAN in Render: data[j].i");
			}
			if (std::isnan(filt[j - start]))
			{
				throw std::runtime_error("NAN in Render: filt");
			}
		}
		//rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Out = %g %g\n", accum.real(), accum.imag());

		//Perform IQ demodulation
		rs::RsComplex ph = exp(rs::RsComplex(0.0, 1.0) * phase);
		out[i] = ph * accum;
	}
	//Return the result
	return boost::shared_array<rs::RsComplex>(out);
}

