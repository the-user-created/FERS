// radar_signal.cpp
// Classes for different types of radar waveforms
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 24 May 2006

#include "radar_signal.h"

#include "core/parameters.h"
#include "core/portable_utils.h"
#include "interpolation/interpolation_filter.h"
#include "math_utils/dsp_filters.h"

using namespace rs;

// =====================================================================================================================
//
// RADAR SIGNAL CLASS
//
// =====================================================================================================================

RadarSignal::RadarSignal(std::string name, const RS_FLOAT power, const RS_FLOAT carrierfreq, const RS_FLOAT length,
                         Signal* signal)
	: _name(std::move(name)), _power(power), _carrierfreq(carrierfreq), _length(length), _signal(signal),
	  _polar(std::complex<RS_FLOAT>(1.0, 0.0), std::complex<RS_FLOAT>(0.0, 0.0))
{
	if (!signal) { throw std::logic_error("RadarSignal cannot be constructed with NULL signal"); }
}

// Get the carrier frequency
RS_FLOAT RadarSignal::getCarrier() const { return _carrierfreq; }

std::shared_ptr<RS_COMPLEX[]> RadarSignal::render(const std::vector<InterpPoint>& points, unsigned& size,
                                                  const RS_FLOAT fracWinDelay) const
{
	std::shared_ptr<RS_COMPLEX[]> data = _signal->render(points, size, fracWinDelay);
	const RS_FLOAT scale = std::sqrt(_power);
	for (unsigned i = 0; i < size; i++) { data[i] *= scale; }
	return data;
}

// =====================================================================================================================
//
// SIGNAL CLASS
//
// =====================================================================================================================

namespace interp_filt
{
	// Initialize the interpolation filter instance
	InterpFilter* InterpFilter::_instance = nullptr;
}

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
	for (unsigned i = 0; i < samples; i++) { _data[i] = RS_COMPLEX(inData[i], 0.0); }
}

void Signal::load(const RS_COMPLEX* inData, const unsigned samples, const RS_FLOAT sampleRate)
{
	clear();
	const unsigned ratio = parameters::oversampleRatio();
	_data = new RS_COMPLEX[samples * ratio];
	_size = samples * ratio;
	_rate = sampleRate * ratio;
	if (ratio == 1) { for (unsigned i = 0; i < samples; i++) { _data[i] = inData[i]; } }
	else { upsample(inData, samples, _data, ratio); }
}

RS_FLOAT* Signal::copyData() const
{
	auto* result = new RS_FLOAT[_size];
	std::memcpy(result, _data, sizeof(RS_FLOAT) * _size);
	return result;
}

std::shared_ptr<RS_COMPLEX[]> Signal::render(const std::vector<InterpPoint>& points, unsigned& size,
                                             const double fracWinDelay) const
{
	// Allocate memory for rendering
	auto out = std::make_unique<RS_COMPLEX[]>(_size);
	size = _size;

	const RS_FLOAT timestep = 1.0 / _rate;
	const int filt_length = static_cast<int>(parameters::renderFilterLength());
	const interp_filt::InterpFilter* interp = interp_filt::InterpFilter::getInstance();
	auto iter = points.begin();
	auto next = iter + 1;

	if (next == points.end()) { next = iter; }

	const RS_FLOAT idelay = round(_rate * iter->delay);

	RS_FLOAT sample_time = iter->time;
	for (int i = 0; i < static_cast<int>(_size); i++)
	{
		// Check if we should move on to the next set of interp points
		if (sample_time > next->time)
		{
			iter = next;
			if (next + 1 != points.end()) { ++next; }
		}

		RS_FLOAT aw = 1, bw = 0;
		if (iter < next)
		{
			bw = (sample_time - iter->time) / (next->time - iter->time);
			aw = 1 - bw;
		}

		RS_FLOAT amplitude = std::sqrt(iter->power) * aw + std::sqrt(next->power) * bw;
		RS_FLOAT phase = iter->phase * aw + next->phase * bw;
		RS_FLOAT fdelay = -((iter->delay * aw + next->delay * bw) * _rate - idelay + fracWinDelay);

		const int i_sample_unwrap = floor(fdelay);
		fdelay -= i_sample_unwrap;
		const RS_FLOAT* filt = interp->getFilter(fdelay);

		const int start = std::max(-filt_length / 2, -i);
		const int end = std::min(filt_length / 2, static_cast<int>(_size) - i);

		RS_COMPLEX accum(0.0, 0.0);
		for (int j = start; j < end; j++)
		{
			if (i + j + i_sample_unwrap >= _size || i + j + i_sample_unwrap < 0 || j + filt_length / 2 >= filt_length)
			{
				continue;
			}

			accum += amplitude * _data[i + j + i_sample_unwrap] * filt[j + filt_length / 2];
		}

		RS_COMPLEX ph = exp(RS_COMPLEX(0.0, 1.0) * phase);
		out[i] = ph * accum;
		sample_time += timestep;
	}

	return std::shared_ptr<RS_COMPLEX[]>(out.release());
}
