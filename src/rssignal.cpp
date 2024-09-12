// signal.cpp
// Class to contain an arbitrary frequency domain signal
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 24 May 2006

#include "rssignal.h"

#include "dsp_filters.h"
#include "interpolation_filter.h"
#include "logging.h"
#include "portable_utils.h"
#include "rsparameters.h"

using namespace signal;
using namespace rs;

namespace interp_filt
{
	// Initialize the interpolation filter instance
	InterpFilter* InterpFilter::_instance = nullptr;
}

void signal::adcSimulate(RS_COMPLEX* data, const unsigned size, const unsigned bits, const RS_FLOAT fullscale)
{
	const RS_FLOAT levels = pow(2, bits - 1);
	for (unsigned it = 0; it < size; it++)
	{
		RS_FLOAT i = std::floor(levels * data[it].real() / fullscale) / levels;
		RS_FLOAT q = std::floor(levels * data[it].imag() / fullscale) / levels;
		// TODO: can use std::clamp
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
		data[it] = RS_COMPLEX(i, q);
	}
}

// =====================================================================================================================
//
// SIGNAL CLASS
//
// =====================================================================================================================

void Signal::clear()
{
	delete[] _data;
	_data = nullptr;
	_size = 0;
	_rate = 0;
}

void Signal::load(const RS_FLOAT* inData, const unsigned samples, const RS_FLOAT sampleRate)
{
	clear();
	_size = samples;
	_rate = sampleRate;
	_data = new RS_COMPLEX[samples];
	for (unsigned i = 0; i < samples; i++)
	{
		_data[i] = RS_COMPLEX(inData[i], 0.0);
	}
}

void Signal::load(const RS_COMPLEX* inData, const unsigned samples, const RS_FLOAT sampleRate)
{
	clear();
	const unsigned ratio = RsParameters::oversampleRatio();
	_data = new RS_COMPLEX[samples * ratio];
	_size = samples * ratio;
	_rate = sampleRate * ratio;
	if (ratio == 1)
	{
		for (unsigned i = 0; i < samples; i++)
		{
			_data[i] = inData[i];
		}
	}
	else
	{
		upsample(inData, samples, _data, ratio);
	}
}

RS_FLOAT* Signal::copyData() const
{
	auto* result = new RS_FLOAT[_size];
	std::memcpy(result, _data, sizeof(RS_FLOAT) * _size);
	return result;
}

boost::shared_array<RS_COMPLEX> Signal::render(const std::vector<InterpPoint>& points, unsigned& size,
                                               const double fracWinDelay) const
{
	// TODO: This needs to be fixed and simplified
	//Allocate memory for rendering
	auto* out = new RS_COMPLEX[_size];
	size = _size;

	//Get the sample interval
	const RS_FLOAT timestep = 1.0 / _rate;
	//Create the rendering window
	const int filt_length = static_cast<int>(RsParameters::renderFilterLength());
	const interp_filt::InterpFilter* interp = interp_filt::InterpFilter::getInstance();
	//Loop through the interp points, rendering each in time
	auto iter = points.begin();
	auto next = iter + 1;
	if (next == points.end())
	{
		next = iter;
	}

	//Get the delay of the first point
	//C Tong: iDelay is in number of receiver samples (possibly with a fractional part)
	const RS_FLOAT idelay = portable_utils::rsRound(_rate * iter->delay);
	//logging::printf(logging::RS_VERY_VERBOSE, "idelay = %g\n", idelay);

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
		RS_COMPLEX accum(0.0, 0.0);

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
				throw std::runtime_error("NaN in Render: data[j].r");
			}
			if (std::isnan(_data[j].imag()))
			{
				throw std::runtime_error("NaN in Render: data[j].i");
			}
			if (std::isnan(filt[j - start]))
			{
				throw std::runtime_error("NaN in Render: filt");
			}
		}
		//logging::printf(logging::RS_VERY_VERBOSE, "Out = %g %g\n", accum.real(), accum.imag());

		//Perform IQ demodulation
		RS_COMPLEX ph = exp(RS_COMPLEX(0.0, 1.0) * phase);
		out[i] = ph * accum;

		// Increment the sample time (in a Clang-Tidy compliant way)
		sample_time += timestep;
	}
	//Return the result
	return boost::shared_array<RS_COMPLEX>(out);
}
