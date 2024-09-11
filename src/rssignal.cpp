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
	RS_FLOAT
	besselI0(const RS_FLOAT x)
	{
		RS_FLOAT i0;
		if (RS_FLOAT t = x / 3.75; t < 0.0)
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
		static RS_FLOAT sinc(RS_FLOAT x);

		/// Compute the value of a Kaiser Window at the given x
		[[nodiscard]] RS_FLOAT kaiserWinCompute(RS_FLOAT x) const;

		/// Calculate the value of the interpolation filter at time x
		[[nodiscard]] RS_FLOAT interpFilter(RS_FLOAT x) const;

		/// Get a pointer to the filter with approximately the specified delay
		[[nodiscard]] const RS_FLOAT* getFilter(RS_FLOAT delay) const;

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

		RS_FLOAT _alpha; //!< 'alpha' parameter
		RS_FLOAT _beta; //!< 'beta' parameter
		RS_FLOAT _bessel_beta; //!< I0(beta)
		int _length;
		int _table_filters; //!< Number of filters in the filter table
		RS_FLOAT* _filter_table; //!< Table of precalculated filters
	};

	/// Interpfilter class constructor
	InterpFilter::InterpFilter()
	{
		_length = static_cast<int>(RsParameters::renderFilterLength());
		//Size of the table to use for interpolation
		_table_filters = 1000;
		//Allocate memory for the table
		_filter_table = new RS_FLOAT[_table_filters * _length];
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
			const RS_FLOAT delay = i / static_cast<RS_FLOAT>(hfilt);
			for (int j = static_cast<int>(-_alpha); j < _alpha; j++)
			{
				_filter_table[static_cast<int>((i + hfilt) * _length + j + _alpha)] = interpFilter(j - delay);
			}
		}
		rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Filter table complete.\n");
	}

	/// Get a pointer to the filter with approximately the specified delay
	const RS_FLOAT* InterpFilter::getFilter(const RS_FLOAT delay) const
	{
		const auto filt = static_cast<unsigned int>((delay + 1) * (_table_filters / 2.0));

		if (delay <= -1 || delay >= 1)
		{
			rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "GetFilter %f %d\n", delay, filt);
			throw std::runtime_error("[BUG] Requested delay filter value out of range");
		}

		return &_filter_table[filt * _length];
	}

	/// Lookup the value of the interpolation filter at time x
	RS_FLOAT InterpFilter::interpFilter(const RS_FLOAT x) const
	{
		const RS_FLOAT w = kaiserWinCompute(x + _alpha);
		const RS_FLOAT s = sinc(x); //The filter value
		const RS_FLOAT filt = w * s;
		// rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "%g %g\n", t, filt);
		return filt;
	}

	/// Compute the sinc function at the specified x
	RS_FLOAT InterpFilter::sinc(const RS_FLOAT x)
	{
		if (x == 0)
		{
			return 1.0;
		}
		return std::sin(x * M_PI) / (x * M_PI);
	}

	/// Compute the value of a Kaiser Window at the given x
	RS_FLOAT InterpFilter::kaiserWinCompute(const RS_FLOAT x) const
	{
		if (x < 0 || x > _alpha * 2)
		{
			return 0;
		}
		return besselI0(_beta * std::sqrt(1 - std::pow((x - _alpha) / _alpha, 2))) / _bessel_beta;
	}

	//Create an instance of InterpFilter
	InterpFilter* InterpFilter::_instance = nullptr;
}

/// Simulate the effect of and ADC converter on the signal
void rs_signal::adcSimulate(Complex* data, const unsigned int size, const unsigned int bits, const RS_FLOAT fullscale)
{
	//Get the number of levels associated with the number of bits
	const RS_FLOAT levels = pow(2, bits - 1);
	for (unsigned int it = 0; it < size; it++)
	{
		//Simulate the ADC effect on the I and Q samples
		RS_FLOAT i = std::floor(levels * data[it].real() / fullscale) / levels;
		RS_FLOAT q = std::floor(levels * data[it].imag() / fullscale) / levels;
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
void Signal::clear()
{
	delete[] _data;
	_size = 0;
	_rate = 0;
	_data = nullptr; //Set data to zero to prevent multiple frees
}

//Load data into the signal, with the given sample rate and size
void Signal::load(const RS_FLOAT* inData, const unsigned int samples, const RS_FLOAT sampleRate)
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
void Signal::load(const Complex* inData, const unsigned int samples, const RS_FLOAT sampleRate)
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
RS_FLOAT Signal::rate() const
{
	return _rate;
}

//Return the size, in samples, of the signal
unsigned int Signal::size() const
{
	return _size;
}

/// Get a copy of the signal domain data
RS_FLOAT* Signal::copyData() const
{
	auto* result = new RS_FLOAT[_size];
	//Copy the data into result
	std::memcpy(result, _data, sizeof(RS_FLOAT) * _size);
	return result;
}

/// Render the pulse with the specified doppler, delay and amplitude
boost::shared_array<RsComplex> Signal::render(const std::vector<InterpPoint>& points,
                                              unsigned int& size, const double fracWinDelay) const
{
	//Allocate memory for rendering
	auto* out = new RsComplex[_size];
	size = _size;

	//Get the sample interval
	const RS_FLOAT timestep = 1.0 / _rate;
	//Create the rendering window
	const int filt_length = static_cast<int>(RsParameters::renderFilterLength());
	const InterpFilter* interp = InterpFilter::getInstance();
	//Loop through the interp points, rendering each in time
	auto iter = points.begin();
	auto next = iter + 1;
	if (next == points.end())
	{
		next = iter;
	}

	//Get the delay of the first point
	//C Tong: iDelay is in number of receiver samples (possibly with a fractional part)
	const RS_FLOAT idelay = rs_portable::rsRound(_rate * iter->delay);
	//rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "idelay = %g\n", idelay);

	//Loop over the pulse, performing the rendering
	RS_FLOAT sample_time = iter->time;
	for (int i = 0; i < static_cast<int>(_size); i++)
	{
		//Check if we should move on to the next set of interp points
		if (sample_time > next->time)
		{
			iter = next;
			if (next + 1 != points.end())
			{
				++next;
			}
		}
		//Get the weightings for the parameters
		RS_FLOAT aw = 1, bw = 0;
		if (iter < next)
		{
			bw = (sample_time - iter->time) / (next->time - iter->time);
			aw = 1 - bw;

			//**C Tong:**
			//bw is fraction of time elapsed between 2 samples (this is interpolated)
			//aw is then the fraction of time remaining
		}

		//Now calculate the current sample parameters
		RS_FLOAT amplitude = std::sqrt(iter->power) * aw + std::sqrt(next->power) * bw;
		RS_FLOAT phase = iter->phase * aw + next->phase * bw;
		RS_FLOAT fdelay = -((iter->delay * aw + next->delay * bw) * _rate - idelay + fracWinDelay);

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

		const RS_FLOAT* filt = interp->getFilter(fdelay);

		//Get the start and end times of interpolation
		int start = -filt_length / 2;
		if (i + start < 0)
		{
			start = -i;
		}

		int end = filt_length / 2;
		if (i + end >= _size)
		{
			end = static_cast<int>(_size) - i;
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
		RsComplex ph = exp(RsComplex(0.0, 1.0) * phase);
		out[i] = ph * accum;

		// Increment the sample time (in a Clang-Tidy compliant way)
		sample_time += timestep;
	}
	//Return the result
	return boost::shared_array<RsComplex>(out);
}
