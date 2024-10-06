// dsp_filters.cpp
// Digital Signal Processing support functions
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 30 July 2007

#include "dsp_filters.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <numeric>
#include <stdexcept>

#include "core/parameters.h"

constexpr RealType BLACKMAN_A0 = 0.42;
constexpr RealType BLACKMAN_A1 = 0.5;
constexpr RealType BLACKMAN_A2 = 0.08;

namespace
{
	constexpr RealType sinc(const RealType x) noexcept { return x == 0 ? 1.0 : std::sin(x * PI) / (x * PI); }

	std::vector<RealType> blackmanFir(const RealType cutoff, unsigned& filtLength)
	{
		filtLength = params::renderFilterLength() * 2;
		std::vector<RealType> coeffs(filtLength); // Use vector for automatic memory management
		const RealType n = filtLength / 2.0;
		std::ranges::generate(coeffs, [&, i = 0]() mutable
		{
			// We use the Blackman window, for a suitable tradeoff between rolloff and stopband attenuation
			// Equivalent Kaiser beta = 7.04 (Oppenhiem and Schaffer, Hamming)
			const RealType filt = sinc(cutoff * (i - n));
			const RealType window = BLACKMAN_A0 - BLACKMAN_A1 * std::cos(PI * i / n) + BLACKMAN_A2 * std::cos(
				2 * PI * i / n);
			return filt * window;
		});
		return coeffs;
	}
}

namespace signal
{
	void upsample(const std::span<const ComplexType> in, std::span<ComplexType> out, const unsigned ratio)
	{
		// TODO: this would be better as a multirate upsampler
		// This implementation is functional but suboptimal.
		// Users requiring higher accuracy should oversample outside FERS until this is addressed.

		// Early check for valid input/output buffer sizes
		if (in.empty() || out.empty() || ratio == 0) { throw std::invalid_argument("Invalid input arguments"); }

		// Compute FIR filter coefficients using Blackman window
		unsigned filt_length = 0;
		const auto coeffs = blackmanFir(1 / static_cast<RealType>(ratio), filt_length);

		// Temporary buffer to hold upsampled and filtered data
		std::vector<ComplexType> tmp(in.size() * ratio + filt_length);

		// Insert input samples at intervals of 'ratio', leaving zeroes in between
		for (size_t i = 0; i < in.size(); ++i) { tmp[i * ratio] = in[i]; }

		// Create and apply the FIR filter
		const FirFilter filt(coeffs);
		filt.filter(tmp); // Automatically handles tmp.size()

		// Output filtered result, correcting for filter delay (filt_length / 2 - 1)
		const auto offset = static_cast<ptrdiff_t>(filt_length / 2 - 1);
		std::ranges::copy_n(tmp.begin() + offset, static_cast<std::ptrdiff_t>(in.size() * ratio), out.begin());
	}

	void downsample(std::span<const ComplexType> in, std::span<ComplexType> out, const unsigned ratio)
	{
		// Early validation of arguments
		if (ratio == 0 || in.empty() || out.empty() || in.size() < ratio)
		{
			throw std::invalid_argument(
				"Invalid input arguments");
		}

		// TODO: Replace with a more efficient multirate downsampling implementation.
		// Compute FIR filter coefficients using Blackman window
		unsigned filt_length = 0;
		const auto coeffs = blackmanFir(1 / static_cast<RealType>(ratio), filt_length);

		// Temporary buffer to hold filtered data, with padding for the filter length
		std::vector<ComplexType> tmp(in.size() + filt_length);

		// Copy input data to temporary buffer, zero-padding the end
		std::ranges::copy(in, tmp.begin());
		std::fill(tmp.begin() + static_cast<std::ptrdiff_t>(in.size()), tmp.end(), ComplexType{0, 0});

		// Create and apply the FIR filter
		const FirFilter filt(coeffs);
		filt.filter(tmp); // Filter the entire tmp buffer

		// Downsample the filtered data and normalize by ratio
		for (size_t i = 0; i < out.size(); ++i)
		{
			out[i] = tmp[i * ratio + filt_length / 2] / static_cast<RealType>(ratio);
		}
	}

	// =================================================================================================================
	//
	// IIRFILTER CLASS
	//
	// =================================================================================================================

	IirFilter::IirFilter(std::span<const RealType> denCoeffs, std::span<const RealType> numCoeffs)
	{
		if (denCoeffs.size() != numCoeffs.size())
		{
			throw std::logic_error("IIRFilter does not currently support mixed order filters");
		}

		_order = denCoeffs.size();
		_a.assign(denCoeffs.begin(), denCoeffs.end());
		_b.assign(numCoeffs.begin(), numCoeffs.end());
		_w.assign(_order, 0.0); // Initialize _w with zeros
	}

	IirFilter::IirFilter(const RealType* denCoeffs, const RealType* numCoeffs, const unsigned order) :
		_a(denCoeffs, denCoeffs + order), _b(numCoeffs, numCoeffs + order), _w(order, 0.0), _order(order) {}

	RealType IirFilter::filter(const RealType sample)
	{
		// Shift the delay line to the right
		std::ranges::rotate(_w, _w.end() - 1); // Rotate right by 1 element

		// Update the first element with the current sample
		_w[0] = sample;

		// Apply the feedback from the denominator coefficients (skip a[0], assuming it's 1)
		for (unsigned j = 1; j < _order; ++j) { _w[0] -= _a[j] * _w[j]; }

		// Calculate the output using numerator coefficients and return
		return std::inner_product(_b.begin(), _b.end(), _w.begin(), 0.0);
	}

	void IirFilter::filter(std::span<RealType> samples)
	{
		for (auto& sample : samples)
		{
			// Shift the delay line to the right
			std::ranges::rotate(_w, _w.end() - 1);

			// Update the first element with the current sample
			_w[0] = sample;

			// Apply the feedback from denominator coefficients
			for (unsigned j = 1; j < _order; ++j) { _w[0] -= _a[j] * _w[j]; }

			// Calculate the output and store in-place
			sample = std::inner_product(_b.begin(), _b.end(), _w.begin(), 0.0);
		}
	}

	// =================================================================================================================
	//
	// FIRFILTER CLASS
	//
	// =================================================================================================================

	void FirFilter::filter(std::span<RealType> samples)
	{
		// See Oppenheim and Scaffer, section 6.5 "Basic Network Structures for FIR Systems"
		// FIR filter using direct form
		// Temporary line buffer for delay line (reuse it across all samples)
		std::vector<RealType> line(_order, 0);

		// Iterate over each sample
		for (auto& sample : samples)
		{
			// Insert the new sample at the start of the line
			line[0] = sample;
			RealType result = 0.0;

			// Perform the convolution (dot product) with the filter coefficients
			for (unsigned j = 0; j < _order; ++j) { result += line[_order - j - 1] * _filter[j]; }

			// Store the result in the current sample
			sample = result;

			// Shift the line buffer manually instead of using std::rotate
			for (unsigned j = _order - 1; j > 0; --j) { line[j] = line[j - 1]; }
		}
	}

	void FirFilter::filter(std::span<ComplexType> samples) const
	{
		// Temporary line buffer for delay line (reuse it across all samples)
		std::vector line(_order, ComplexType{0.0, 0.0});

		// Iterate over each complex sample
		for (auto& sample : samples)
		{
			// Insert the new sample at the start of the line
			line[0] = sample;
			ComplexType result(0.0, 0.0);

			// Perform the convolution (dot product) with the filter coefficients
			for (unsigned j = 0; j < _order; ++j) { result += line[_order - j - 1] * _filter[j]; }

			// Store the result in the current sample
			sample = result;

			// Shift the line buffer manually
			for (unsigned j = _order - 1; j > 0; --j) { line[j] = line[j - 1]; }
		}
	}

	// =================================================================================================================
	//
	// ARFILTER CLASS
	//
	// =================================================================================================================

	// Private helper method to apply the filter logic
	RealType ArFilter::applyFilter(const RealType sample)
	{
		// Manually shift the delay line (_w) right by one position
		for (unsigned j = _order - 1; j > 0; --j) { _w[j] = _w[j - 1]; }
		_w[0] = sample;

		// Apply the filter using the coefficients
		for (unsigned j = 1; j < _order; ++j) { _w[0] -= _filter[j] * _w[j]; }

		return _w[0];
	}

	RealType ArFilter::filter(const RealType sample) { return applyFilter(sample); }

	void ArFilter::filter(std::span<RealType> samples)
	{
		std::ranges::transform(samples, samples.begin(), [this](const RealType sample) { return applyFilter(sample); });
	}

	// =================================================================================================================
	//
	// UPSAMPLER CLASS
	//
	// =================================================================================================================

	Upsampler::Upsampler(const int ratio) : _ratio(ratio), _filter_size(8 * ratio + 1), _filterbank(_filter_size),
	                                        _sample_memory(_filter_size / ratio + 1, 0.0)
	{
		_filterbank.resize(_filter_size);
		_sample_memory.resize(_filter_size / ratio + 1, 0.0); // Initialize with zeros

		for (int i = 0; i < _filter_size; i++)
		{
			RealType window_value = 0.54 - 0.46 * std::cos(2 * PI * i / static_cast<RealType>(_filter_size));
			RealType filter_value = sinc(1.0 / static_cast<RealType>(ratio) * (i - _filter_size / 2.0));
			_filterbank[i] = filter_value * window_value;
		}
	}

	void Upsampler::upsample(std::span<const RealType> inSamples, std::span<RealType> outSamples)
	{
		if (outSamples.size() != _ratio * inSamples.size())
		{
			throw std::runtime_error("Target array size is not correct in Upsample");
		}
		// Polyphase upsampler implementation
		// See fers_upsample_p.m in the documentation for more details
		// Follows the diagram in section 4.7.4 "Polyphase Implementation of Interpolation Filters" of
		// Discrete Time Signal Processing, 2nd ed., Oppenheim and Schafer
		int branch = 0;
		for (size_t i = 0; i < inSamples.size(); ++i, ++branch)
		{
			if (branch >= _ratio) { branch = 0; }
			outSamples[i] = 0;

			for (int j = branch; j < _filter_size; j += _ratio)
			{
				outSamples[i] += _filterbank[j] * getSample(inSamples, static_cast<long>(i - j) / _ratio);
			}
		}

		// Transfer last samples into sample memory
		if (const unsigned transfer_size = _filter_size / _ratio + 1; inSamples.size() >= transfer_size)
		{
			std::copy_n(inSamples.end() - transfer_size, transfer_size, _sample_memory.begin());
		}
		else
		{
			std::move(_sample_memory.begin() + static_cast<std::ptrdiff_t>(inSamples.size()), _sample_memory.end(),
			          _sample_memory.begin());
			std::copy_n(inSamples.begin(), inSamples.size(),
			            _sample_memory.begin() + (transfer_size - static_cast<std::ptrdiff_t>(inSamples.size())));
		}
	}

	// =================================================================================================================
	//
	// DECADE UPSAMPLER CLASS
	//
	// =================================================================================================================

	DecadeUpsampler::DecadeUpsampler()
	{
		/// 11th order elliptic lowpass at 0.1fs
		constexpr std::array den_coeffs = {
			1.0, -10.301102119865, 48.5214567642597, -137.934509572412, 262.914952985445,
			-352.788381841481, 340.027874008585, -235.39260470286, 114.698499845697,
			-37.4634653062448, 7.38208765922137, -0.664807695826097
		};

		constexpr std::array num_coeffs = {
			2.7301694322809e-06, -1.8508123430239e-05, 5.75739466753894e-05, -0.000104348734423658,
			0.000111949190289715, -4.9384188225528e-05, -4.9384188225522e-05, 0.00011194919028971,
			-0.000104348734423656, 5.75739466753884e-05, -1.85081234302388e-05, 2.73016943228086e-06
		};

		_filter = std::make_unique<IirFilter>(den_coeffs.data(), num_coeffs.data(), den_coeffs.size());
	}

	void DecadeUpsampler::upsample(const RealType sample, std::span<RealType> out) const
	{
		if (out.size() != 10) { throw std::invalid_argument("Output span must have a size of 10."); }
		out[0] = sample;
		// Fill the rest of the span with zeros
		std::fill(out.begin() + 1, out.end(), 0);
		_filter->filter(out);
	}

	void DecadeUpsampler::upsample(const std::span<const RealType> in, std::span<RealType> out) const
	{
		if (out.size() != in.size() * 10)
		{
			throw std::invalid_argument("Output span size must be 10 times the input size.");
		}

		for (size_t i = 0; i < in.size(); ++i)
		{
			out[i * 10] = in[i];
			// Fill the rest of the 10-sample block with zeros
			std::fill(out.begin() + static_cast<std::ptrdiff_t>(i) * 10 + 1,
			          out.begin() + (static_cast<std::ptrdiff_t>(i) + 1) * 10, 0);
		}
		_filter->filter(out);
	}
}
