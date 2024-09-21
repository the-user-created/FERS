// radar_signal.cpp
// Classes for different types of radar waveforms
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// Started: 24 May 2006

#include "radar_signal.h"

#include "dsp_filters.h"
#include "core/parameters.h"
#include "interpolation/interpolation_filter.h"
#include "interpolation/interpolation_point.h"

namespace signal
{
	// =================================================================================================================
	//
	// RADAR SIGNAL CLASS
	//
	// =================================================================================================================

	RadarSignal::RadarSignal(std::string name, const RealType power, const RealType carrierfreq, const RealType length,
	                         std::unique_ptr<Signal> signal)
		: _name(std::move(name)), _power(power), _carrierfreq(carrierfreq), _length(length), _signal(std::move(signal)),
		  _polar(std::complex(1.0, 0.0), std::complex(0.0, 0.0))
	{
		// Check if the signal is empty
		if (!_signal) { throw std::runtime_error("Signal is empty"); }
	}

	// Get the carrier frequency
	RealType RadarSignal::getCarrier() const { return _carrierfreq; }

	std::vector<ComplexType> RadarSignal::render(const std::vector<interp::InterpPoint>& points, unsigned& size,
	                                            const RealType fracWinDelay) const
	{
		std::vector<ComplexType> data = _signal->render(points, size, fracWinDelay);
		const RealType scale = std::sqrt(_power);
		for (unsigned i = 0; i < size; i++) { data[i] *= scale; }
		return data;
	}

	// =================================================================================================================
	//
	// SIGNAL CLASS
	//
	// =================================================================================================================

	void Signal::clear()
	{
		_size = 0;
		_rate = 0;
	}

	void Signal::load(const RealType* inData, const unsigned samples, const RealType sampleRate)
	{
		clear();
		_size = samples;
		_rate = sampleRate;
		_data.resize(samples);
		for (unsigned i = 0; i < samples; i++) { _data[i] = ComplexType(inData[i], 0.0); }
	}

	void Signal::load(const ComplexType* inData, const unsigned samples, const RealType sampleRate)
	{
		clear();
		const unsigned ratio = params::oversampleRatio();
		_data.resize(samples * ratio);
		_size = samples * ratio;
		_rate = sampleRate * ratio;
		if (ratio == 1) { for (unsigned i = 0; i < samples; i++) { _data[i] = inData[i]; } }
		else { upsample(inData, samples, _data.data(), ratio); }
	}

	std::vector<RealType> Signal::copyData() const
	{
		std::vector<RealType> result(_size);
		for (unsigned i = 0; i < _size; ++i) { result[i] = _data[i].real(); }
		return result;
	}

	std::vector<ComplexType> Signal::render(const std::vector<interp::InterpPoint>& points, unsigned& size,
	                                       const double fracWinDelay) const
	{
		auto out = std::vector<ComplexType>(_size);
		size = _size;

		const RealType timestep = 1.0 / _rate;
		const int filt_length = static_cast<int>(params::renderFilterLength());
		const auto* interp = interp::InterpFilter::getInstance();

		auto iter = points.begin();
		auto next = points.size() > 1 ? std::next(iter) : iter;
		const RealType idelay = std::round(_rate * iter->delay);
		RealType sample_time = iter->time;

		for (int i = 0; i < static_cast<int>(_size); ++i)
		{
			if (sample_time > next->time && next != iter)
			{
				iter = next;
				if (std::next(next) != points.end()) { ++next; }
			}

			auto [amplitude, phase, fdelay, i_sample_unwrap] = calculateWeightsAndDelays(
				iter, next, sample_time, idelay, fracWinDelay);
			const RealType* filt = interp->getFilter(fdelay);
			ComplexType accum = performConvolution(i, filt, filt_length, amplitude, i_sample_unwrap);
			out[i] = std::exp(ComplexType(0.0, 1.0) * phase) * accum;

			sample_time += timestep;
		}

		return out;
	}

	std::tuple<RealType, RealType, RealType, int> Signal::calculateWeightsAndDelays(
		const std::vector<interp::InterpPoint>::const_iterator iter,
		const std::vector<interp::InterpPoint>::const_iterator next,
		const RealType sampleTime, const RealType idelay, const RealType fracWinDelay) const
	{
		const RealType bw = iter < next ? (sampleTime - iter->time) / (next->time - iter->time) : 0.0;
		const RealType aw = 1.0 - bw;

		const RealType amplitude = std::sqrt(iter->power) * aw + std::sqrt(next->power) * bw;
		const RealType phase = iter->phase * aw + next->phase * bw;
		RealType fdelay = -((iter->delay * aw + next->delay * bw) * _rate - idelay + fracWinDelay);

		const int i_sample_unwrap = static_cast<int>(std::floor(fdelay));
		fdelay -= i_sample_unwrap;

		return {amplitude, phase, fdelay, i_sample_unwrap};
	}

	ComplexType Signal::performConvolution(const int i, const RealType* filt, const int filtLength,
	                                      const RealType amplitude,
	                                      const int iSampleUnwrap) const
	{
		const int start = std::max(-filtLength / 2, -i);
		const int end = std::min(filtLength / 2, static_cast<int>(_size) - i);

		ComplexType accum(0.0, 0.0);

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
}
