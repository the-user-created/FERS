// dsp_filters.cpp
// Digital Signal Processing support functions
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 30 July 2007

#include "dsp_filters.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <numeric>
#include <stdexcept>

#include "core/parameters.h"

using namespace rs;

constexpr RS_FLOAT BLACKMAN_A0 = 0.42;
constexpr RS_FLOAT BLACKMAN_A1 = 0.5;
constexpr RS_FLOAT BLACKMAN_A2 = 0.08;

namespace
{
	RS_FLOAT sinc(const RS_FLOAT x) { return x == 0 ? 1.0 : std::sin(x * M_PI) / (x * M_PI); }

	std::vector<RS_FLOAT> blackmanFir(const RS_FLOAT cutoff, unsigned& filtLength)
	{
		filtLength = parameters::renderFilterLength() * 2;
		std::vector<RS_FLOAT> coeffs(filtLength);  // Use vector for automatic memory management
		const RS_FLOAT n = filtLength / 2.0;
		for (unsigned i = 0; i < filtLength; i++)
		{
			// We use the Blackman window, for a suitable tradeoff between rolloff and stopband attenuation
			// Equivalent Kaiser beta = 7.04 (Oppenhiem and Schaffer, Hamming)
			const RS_FLOAT filt = sinc(cutoff * (i - n));
			const RS_FLOAT window = BLACKMAN_A0 - BLACKMAN_A1 * cos(M_PI * i / n) + BLACKMAN_A2 * cos(2 * M_PI * i / n);
			coeffs[i] = filt * window;
		}
		return coeffs;
	}
}

namespace rs
{
	void upsample(const RS_COMPLEX* in, const unsigned size, RS_COMPLEX* out, const unsigned ratio)
	{
		// TODO: this would be better as a multirate upsampler
		// This implementation is functional but suboptimal.
		// Users requiring higher accuracy should oversample outside FERS until this is addressed.
		unsigned filt_length;
		const auto coeffs = blackmanFir(1 / static_cast<RS_FLOAT>(ratio), filt_length);

		// Temporary buffer to hold upsampled and filtered data
		std::vector<RS_COMPLEX> tmp(size * ratio + filt_length);

		// Insert the input samples at intervals of 'ratio', leaving zeroes in between
		for (unsigned i = 0; i < size; ++i) { tmp[i * ratio] = in[i]; }

		// Create the FIR filter and apply it
		const FirFilter filt(coeffs);
		filt.filter(tmp, size * ratio + filt_length);

		// Output the filtered result with appropriate offset for the filter delay (filt_length / 2 - 1)
		std::copy_n(tmp.begin() + filt_length / 2 - 1, size * ratio, out);
	}

	void downsample(const std::vector<RS_COMPLEX>& in, const unsigned size, std::vector<RS_COMPLEX>& out, const unsigned ratio)
	{
		if (ratio == 0 || size == 0 || !in.data() || !out.data()) { throw std::invalid_argument("Invalid input arguments"); }

		// TODO: Replace with a more efficient multirate downsampling implementation.
		unsigned filt_length = 0;

		const auto coeffs = blackmanFir(1 / static_cast<RS_FLOAT>(ratio), filt_length);

		// Use std::vector for temporary buffer allocation
		std::vector<RS_COMPLEX> tmp(size + filt_length);

		// Initialize the tail of the temporary buffer with zeros (auto-handled by vector initialization).
		std::fill(tmp.begin() + size, tmp.end(), RS_COMPLEX{0, 0});

		// Copy input data to the temporary buffer
		std::copy_n(in.data(), size, tmp.begin());

		// FirFilter class usage
		const FirFilter filt(coeffs);
		filt.filter(tmp, size + filt_length);

		// Downsample the filtered data
		for (unsigned i = 0; i < size / ratio; ++i)
		{
			out[i] = tmp[i * ratio + filt_length / 2] / static_cast<RS_FLOAT>(ratio);
		}
	}
}

// =====================================================================================================================
//
// IIRFILTER CLASS
//
// =====================================================================================================================


IirFilter::IirFilter(const std::vector<RS_FLOAT>& denCoeffs, const std::vector<RS_FLOAT>& numCoeffs)
{
	if (denCoeffs.size() != numCoeffs.size())
	{
		throw std::logic_error("IIRFilter does not currently support mixed order filters");
	}

	_order = denCoeffs.size();

	// Initialize _a, _b, and _w using vectors (which automatically manage memory)
	_a = denCoeffs;
	_b = numCoeffs;
	_w.assign(_order, 0.0); // Initialize _w to be a vector of zeros of the same size
}

IirFilter::IirFilter(const RS_FLOAT* denCoeffs, const RS_FLOAT* numCoeffs, const unsigned order)
	: _a(denCoeffs, denCoeffs + order), // Initialize _a by copying from denCoeffs
	  _b(numCoeffs, numCoeffs + order), // Initialize _b by copying from numCoeffs
	  _w(order, 0.0), // Initialize _w with zeros
	  _order(order) {}

RS_FLOAT IirFilter::filter(const RS_FLOAT sample)
{
	// Shift delay line (state) to the right
	std::rotate(_w.rbegin(), _w.rbegin() + 1, _w.rend());

	// Update the first element of the delay line
	_w[0] = sample;

	// Apply the feedback from denominator coefficients
	for (unsigned j = 1; j < _order; ++j) { _w[0] -= _a[j] * _w[j]; }

	// Calculate the output using numerator coefficients
	return std::inner_product(_b.begin(), _b.end(), _w.begin(), 0.0);
}

void IirFilter::filter(std::vector<RS_FLOAT>& samples, const int size)
{
	for (int i = 0; i < size; ++i)
	{
		// Shift delay line (state) to the right
		std::rotate(_w.rbegin(), _w.rbegin() + 1, _w.rend());

		// Update the first element of the delay line
		_w[0] = samples[i];

		// Apply the feedback from denominator coefficients
		for (unsigned j = 1; j < _order; ++j) { _w[0] -= _a[j] * _w[j]; }

		// Calculate the output using numerator coefficients
		samples[i] = std::inner_product(_b.begin(), _b.end(), _w.begin(), 0.0);
	}
}

// =====================================================================================================================
//
// FIRFILTER CLASS
//
// =====================================================================================================================

void FirFilter::filter(std::vector<RS_FLOAT>& samples, const int size)
{
	// See Oppenheim and Scaffer, section 6.5 "Basic Network Structures for FIR Systems"
	// TODO: Implement one of the more efficient FIR filter forms
	// FIR filter using direct form
	std::vector<RS_FLOAT> line(_order, 0); // Initialize the line with zeros
	for (int i = 0; i < size; i++)
	{
		line[0] = samples[i]; // New sample input at the start of the line
		RS_FLOAT res = 0;

		// Convolve the current line with the filter coefficients
		for (unsigned j = 0; j < _order; j++) { res += line[_order - j - 1] * _filter[j]; }

		samples[i] = res; // Store the filtered result back to the samples

		// Use std::rotate to shift the line buffer
		std::rotate(line.rbegin(), line.rbegin() + 1, line.rend());
	}
}

void FirFilter::filter(std::vector<RS_COMPLEX>& samples, const unsigned size) const
{
	std::vector<std::complex<RS_FLOAT>> line(_order, {0.0, 0.0}); // Zero-initialize the line buffer

	for (unsigned i = 0; i < size; i++)
	{
		line[0] = samples[i]; // New complex sample input at the start of the line
		std::complex<RS_FLOAT> res(0.0, 0.0); // Result accumulator

		// Convolve the current line with the filter coefficients
		for (unsigned j = 0; j < _order; j++) { res += line[_order - j - 1] * _filter[j]; }

		samples[i] = res; // Store the filtered result back to the samples

		// Use std::rotate to shift the line buffer
		std::rotate(line.rbegin(), line.rbegin() + 1, line.rend());
	}
}

// =====================================================================================================================
//
// ARFILTER CLASS
//
// =====================================================================================================================

RS_FLOAT ArFilter::filter(const RS_FLOAT sample)
{
	// Shift _w elements (use std::rotate)
	std::rotate(_w.rbegin(), _w.rbegin() + 1, _w.rend());
	_w[0] = sample;

	// Apply the filter using the coefficients
	for (unsigned j = 1; j < _order; ++j) { _w[0] -= _filter[j] * _w[j]; }

	return _w[0];
}

void ArFilter::filter(std::vector<RS_FLOAT>& samples, const int size)
{
	for (int i = 0; i < size; ++i)
	{
		// Shift _w elements (use std::rotate)
		std::rotate(_w.rbegin(), _w.rbegin() + 1, _w.rend());
		_w[0] = samples[i];

		// Apply the filter using the coefficients
		for (unsigned j = 1; j < _order; ++j) { _w[0] -= _filter[j] * _w[j]; }

		samples[i] = _w[0]; // Store the result back in the sample array
	}
}

// =====================================================================================================================
//
// UPSAMPLER CLASS
//
// =====================================================================================================================

Upsampler::Upsampler(const int ratio) : _ratio(ratio), _filter_size(8 * ratio + 1)
{
	_filterbank.resize(_filter_size);
	_sample_memory.resize(_filter_size / ratio + 1, 0.0); // Initialize with zeros

	for (int i = 0; i < _filter_size; i++)
	{
		const RS_FLOAT window_value = 0.54 - 0.46 * std::cos(2 * M_PI * i / static_cast<RS_FLOAT>(_filter_size));
		const RS_FLOAT filter_value = sinc(1.0 / static_cast<RS_FLOAT>(ratio) * (i - _filter_size / 2.0));
		_filterbank[i] = filter_value * window_value;
	}
}

void Upsampler::upsample(const RS_FLOAT* inSamples, const int inSize, RS_FLOAT* outSamples, const int outSize)
{
	if (outSize != _ratio * inSize) { throw std::runtime_error("Target array size is not correct in Upsample"); }
	// Polyphase upsampler implementation
	// See fers_upsample_p.m in the documentation for more details
	// Follows the diagram in section 4.7.4 "Polyphase Implementation of Interpolation Filters" of
	// Discrete Time Signal Processing, 2nd ed., Oppenheim and Schafer
	for (int i = 0, branch = 0; i < inSize; i++, branch++)
	{
		if (branch >= _ratio) { branch = 0; }
		outSamples[i] = 0;
		for (int j = branch; j < _filter_size; j += _ratio)
		{
			outSamples[i] += _filterbank[j] * getSample(inSamples, i - j / _ratio);
		}
	}

	if (const int transfer_size = _filter_size / _ratio + 1; inSize >= transfer_size)
	{
		std::copy(&inSamples[inSize - transfer_size], &inSamples[inSize], _sample_memory.begin());
	}
	else
	{
		std::copy(_sample_memory.begin() + inSize, _sample_memory.end(), _sample_memory.begin());
		std::copy_n(inSamples, inSize, _sample_memory.begin() + (transfer_size - inSize));
	}
}

// =====================================================================================================================
//
// DECADE UPSAMPLER CLASS
//
// =====================================================================================================================

DecadeUpsampler::DecadeUpsampler()
{
	/// 11th order elliptic lowpass at 0.1fs
	constexpr std::array<RS_FLOAT, 12> den_coeffs = {
		1.0, -10.301102119865, 48.5214567642597, -137.934509572412, 262.914952985445,
		-352.788381841481, 340.027874008585, -235.39260470286, 114.698499845697,
		-37.4634653062448, 7.38208765922137, -0.664807695826097
	};

	constexpr std::array<RS_FLOAT, 12> num_coeffs = {
		2.7301694322809e-06, -1.8508123430239e-05, 5.75739466753894e-05, -0.000104348734423658,
		0.000111949190289715, -4.9384188225528e-05, -4.9384188225522e-05, 0.00011194919028971,
		-0.000104348734423656, 5.75739466753884e-05, -1.85081234302388e-05, 2.73016943228086e-06
	};

	_filter = std::make_unique<IirFilter>(den_coeffs.data(), num_coeffs.data(), den_coeffs.size());
}

void DecadeUpsampler::upsample(const RS_FLOAT sample, std::vector<RS_FLOAT>& out) const
{
	out[0] = sample;
	// Use std::fill to fill the rest of the array with 0s
	std::fill(out.data() + 1, out.data() + 10, 0);
	_filter->filter(out, 10);
}

void DecadeUpsampler::upsample(const std::vector<RS_FLOAT>& in, const int count, std::vector<RS_FLOAT>& out) const
{
	for (int i = 0; i < count; ++i)
	{
		out[i * 10] = in[i];
		// Use std::fill to zero-fill the rest of the 10-sample block
		std::fill(out.data() + i * 10 + 1, out.data() + (i + 1) * 10, 0);
	}
	_filter->filter(out, count * 10);
}
