// radar_signal.cpp
// Classes for different types of radar waveforms
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 24 May 2006

#include "radar_signal.h"

#include "core/parameters.h"
#include "core/portable_utils.h"
#include "interpolation/interpolation_filter.h"
#include "signal_processing/dsp_filters.h"

using namespace rs;

// =====================================================================================================================
//
// RADAR SIGNAL CLASS
//
// =====================================================================================================================

RadarSignal::RadarSignal(std::string name, const RS_FLOAT power, const RS_FLOAT carrierfreq, const RS_FLOAT length,
                         std::unique_ptr<Signal> signal)
	: _name(std::move(name)), _power(power), _carrierfreq(carrierfreq), _length(length), _signal(std::move(signal)),
	  _polar(std::complex(1.0, 0.0), std::complex(0.0, 0.0))
{
	// Check if the signal is empty
	if (!_signal) { throw std::runtime_error("Signal is empty"); }
}

// Get the carrier frequency
RS_FLOAT RadarSignal::getCarrier() const { return _carrierfreq; }

std::vector<RS_COMPLEX> RadarSignal::render(const std::vector<InterpPoint>& points, unsigned& size,
                                                  const RS_FLOAT fracWinDelay) const
{
	std::vector<RS_COMPLEX> data = _signal->render(points, size, fracWinDelay);
	const RS_FLOAT scale = std::sqrt(_power);
	for (unsigned i = 0; i < size; i++) { data[i] *= scale; }
	return data;
}

// =====================================================================================================================
//
// SIGNAL CLASS
//
// =====================================================================================================================

void Signal::clear()
{
	_size = 0;
	_rate = 0;
}

void Signal::load(const RS_FLOAT* inData, const unsigned samples, const RS_FLOAT sampleRate)
{
	clear();
	_size = samples;
	_rate = sampleRate;
	_data.resize(samples);
	for (unsigned i = 0; i < samples; i++) { _data[i] = RS_COMPLEX(inData[i], 0.0); }
}

void Signal::load(const RS_COMPLEX* inData, const unsigned samples, const RS_FLOAT sampleRate)
{
	clear();
	const unsigned ratio = parameters::oversampleRatio();
	_data.resize(samples * ratio);
	_size = samples * ratio;
	_rate = sampleRate * ratio;
	if (ratio == 1) { for (unsigned i = 0; i < samples; i++) { _data[i] = inData[i]; } }
	else { upsample(inData, samples, _data.data(), ratio); }
}

std::vector<RS_FLOAT> Signal::copyData() const
{
	std::vector<RS_FLOAT> result(_size);
	for (unsigned i = 0; i < _size; ++i) {
		result[i] = _data[i].real();
	}
	return result;
}

std::vector<RS_COMPLEX> Signal::render(const std::vector<InterpPoint>& points, unsigned& size,
                                             const double fracWinDelay) const
{
	auto out = std::vector<RS_COMPLEX>(_size);
	size = _size;

	const RS_FLOAT timestep = 1.0 / _rate;
	const int filt_length = static_cast<int>(parameters::renderFilterLength());
	const auto* interp = interp_filt::InterpFilter::getInstance();

	auto iter = points.begin();
	auto next = points.size() > 1 ? std::next(iter) : iter;
	const RS_FLOAT idelay = std::round(_rate * iter->delay);
	RS_FLOAT sample_time = iter->time;

	for (int i = 0; i < static_cast<int>(_size); ++i)
	{
		if (sample_time > next->time && next != iter)
		{
			iter = next;
			if (std::next(next) != points.end()) { ++next; }
		}

		auto [amplitude, phase, fdelay, i_sample_unwrap] = calculateWeightsAndDelays(
			iter, next, sample_time, idelay, fracWinDelay);
		const RS_FLOAT* filt = interp->getFilter(fdelay);
		RS_COMPLEX accum = performConvolution(i, filt, filt_length, amplitude, i_sample_unwrap);
		out[i] = std::exp(RS_COMPLEX(0.0, 1.0) * phase) * accum;

		sample_time += timestep;
	}

	return out;
}

std::tuple<RS_FLOAT, RS_FLOAT, RS_FLOAT, int> Signal::calculateWeightsAndDelays(
	const std::vector<InterpPoint>::const_iterator iter, const std::vector<InterpPoint>::const_iterator next,
	const RS_FLOAT sampleTime, const RS_FLOAT idelay, const RS_FLOAT fracWinDelay) const
{
	const RS_FLOAT bw = iter < next ? (sampleTime - iter->time) / (next->time - iter->time) : 0.0;
	const RS_FLOAT aw = 1.0 - bw;

	const RS_FLOAT amplitude = std::sqrt(iter->power) * aw + std::sqrt(next->power) * bw;
	const RS_FLOAT phase = iter->phase * aw + next->phase * bw;
	RS_FLOAT fdelay = -((iter->delay * aw + next->delay * bw) * _rate - idelay + fracWinDelay);

	const int i_sample_unwrap = static_cast<int>(std::floor(fdelay));
	fdelay -= i_sample_unwrap;

	return {amplitude, phase, fdelay, i_sample_unwrap};
}

RS_COMPLEX Signal::performConvolution(const int i, const RS_FLOAT* filt, const int filtLength, const RS_FLOAT amplitude,
                                      const int iSampleUnwrap) const
{
	const int start = std::max(-filtLength / 2, -i);
	const int end = std::min(filtLength / 2, static_cast<int>(_size) - i);

	RS_COMPLEX accum(0.0, 0.0);

	for (int j = start; j < end; ++j)
	{
		if (const unsigned sample_idx = i + j + iSampleUnwrap;
			sample_idx < _size && j + filtLength / 2 < filtLength)
		{
			accum += amplitude * _data[sample_idx] * filt[j + filtLength / 2];
		}
	}

	return accum;
}
