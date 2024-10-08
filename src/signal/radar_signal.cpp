/**
 * @file radar_signal.cpp
 * @brief Classes for handling radar waveforms and signals.
 *
 * @authors David Young, Marc Brooker
 * @date 2006-06-08
 */

#include "radar_signal.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <iterator>
#include <stdexcept>
#include <utility>

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
		: _name(std::move(name)), _power(power), _carrierfreq(carrierfreq), _length(length), _signal(std::move(signal))
	{
		// Check if the signal is empty
		if (!_signal) { throw std::runtime_error("Signal is empty"); }
	}

	std::vector<ComplexType> RadarSignal::render(const std::vector<interp::InterpPoint>& points, unsigned& size,
	                                             const RealType fracWinDelay) const
	{
		auto data = _signal->render(points, size, fracWinDelay);
		const RealType scale = std::sqrt(_power);

		std::ranges::for_each(data, [scale](auto& value) { value *= scale; });

		return data;
	}

	// =================================================================================================================
	//
	// SIGNAL CLASS
	//
	// =================================================================================================================

	void Signal::clear() noexcept
	{
		_size = 0;
		_rate = 0;
	}

	// Note: This function is not used in the codebase
	/*void Signal::load(std::span<const RealType> inData, const unsigned samples, const RealType sampleRate)
	{
		clear();
		_size = samples;
		_rate = sampleRate;
		_data.resize(samples);
		std::ranges::transform(inData, _data.begin(), [](const RealType value) { return ComplexType(value, 0.0); });
	}*/

	void Signal::load(std::span<const ComplexType> inData, const unsigned samples, const RealType sampleRate)
	{
		clear();
		const unsigned ratio = params::oversampleRatio();
		_data.resize(samples * ratio);
		_size = samples * ratio;
		_rate = sampleRate * ratio;

		if (ratio == 1) { std::ranges::copy(inData, _data.begin()); }
		else { upsample(inData.data(), samples, _data.data(), ratio); }
	}

	std::vector<ComplexType> Signal::render(const std::vector<interp::InterpPoint>& points, unsigned& size,
	                                        const double fracWinDelay) const
	{
		auto out = std::vector<ComplexType>(_size);
		size = _size;

		const RealType timestep = 1.0 / _rate;
		const int filt_length = static_cast<int>(params::renderFilterLength());
		const auto& interp = interp::InterpFilter::getInstance();

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
			const auto& filt = interp.getFilter(fdelay);
			ComplexType accum = performConvolution(i, filt.data(), filt_length, amplitude, i_sample_unwrap);
			out[i] = std::exp(ComplexType(0.0, 1.0) * phase) * accum;

			sample_time += timestep;
		}

		return out;
	}

	constexpr std::tuple<RealType, RealType, RealType, int> Signal::calculateWeightsAndDelays(
		const std::vector<interp::InterpPoint>::const_iterator iter,
		const std::vector<interp::InterpPoint>::const_iterator next, const RealType sampleTime, const RealType idelay,
		const RealType fracWinDelay) const noexcept
	{
		const RealType bw = iter < next ? (sampleTime - iter->time) / (next->time - iter->time) : 0.0;

		const RealType amplitude = std::lerp(std::sqrt(iter->power), std::sqrt(next->power), bw);
		const RealType phase = std::lerp(iter->phase, next->phase, bw);
		RealType fdelay = -(std::lerp(iter->delay, next->delay, bw) * _rate - idelay + fracWinDelay);

		const int i_sample_unwrap = static_cast<int>(std::floor(fdelay));
		fdelay -= i_sample_unwrap;

		return {amplitude, phase, fdelay, i_sample_unwrap};
	}

	ComplexType Signal::performConvolution(const int i, const RealType* filt, const int filtLength,
	                                       const RealType amplitude, const int iSampleUnwrap) const noexcept
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
