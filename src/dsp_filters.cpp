// dsp_filters.cpp
// Digital Signal Processing support functions
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 30 July 2007

#include "dsp_filters.h"

#include <cmath>
#include <cstring>
#include <stdexcept>

#include "logging.h"
#include "parameters.h"

using namespace rs;

namespace
{
	RS_FLOAT sinc(const RS_FLOAT x)
	{
		return x == 0 ? 1.0 : std::sin(x * M_PI) / (x * M_PI);
	}

	RS_FLOAT* blackmanFir(const RS_FLOAT cutoff, unsigned& filtLength)
	{
		filtLength = parameters::renderFilterLength() * 2;
		auto* coeffs = new RS_FLOAT[filtLength];
		const RS_FLOAT n = filtLength / 2.0;
		for (int i = 0; i < filtLength; i++)
		{
			// We use the Blackman window, for a suitable tradeoff between rolloff and stopband attenuation
			// Equivalent Kaiser beta = 7.04 (Oppenhiem and Schaffer, Hamming)
			const RS_FLOAT filt = sinc(cutoff * (i - n));
			const RS_FLOAT window = 0.42 - 0.5 * cos(M_PI * i / n) + 0.08 * cos(2 * M_PI * i / n);
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
		// In fact, the whole scheme is currently sub-optimal - we could use better filters, better windows and a better approach
		// it works ok for now, users seeking higher accuracy can oversample outside FERS until this is fixed
		unsigned filt_length;
		const RS_FLOAT* coeffs = blackmanFir(1 / static_cast<RS_FLOAT>(ratio), filt_length);
		auto* tmp = new RS_COMPLEX[size * ratio + filt_length]();
		// TODO: can use std::copy
		for (int i = 0; i < size; i++)
		{
			tmp[i * ratio] = in[i];
		}
		const FirFilter filt(coeffs, filt_length);
		filt.filter(tmp, size * ratio + filt_length);
		for (int i = 0; i < size * ratio; i++)
		{
			out[i] = tmp[i + filt_length / 2 - 1];
		}
		delete[] tmp;
		delete[] coeffs;
	}

	void downsample(const RS_COMPLEX* in, const unsigned size, RS_COMPLEX* out, const unsigned ratio)
	{
		// TODO: This would be better (and much faster) as a multirate downsampler
		unsigned filt_length;
		const RS_FLOAT* coeffs = blackmanFir(1 / static_cast<RS_FLOAT>(ratio), filt_length);
		auto* tmp = new RS_COMPLEX[size + filt_length];
		// TODO: can use std::copy
		for (unsigned i = size - 1; i < size + filt_length; i++)
		{
			tmp[i] = 0;
		}

		for (int i = 0; i < size; i++)
		{
			tmp[i] = in[i];
		}
		const FirFilter filt(coeffs, filt_length);
		filt.filter(tmp, size + filt_length);
		for (int i = 0; i < size / ratio; i++)
		{
			out[i] = tmp[i * ratio + filt_length / 2] / static_cast<RS_FLOAT>(ratio);
		}
		delete[] coeffs;
		delete[] tmp;
	}
}

// =====================================================================================================================
//
// IIRFILTER CLASS
//
// =====================================================================================================================

IirFilter::IirFilter(const std::vector<RS_FLOAT>& denCoeffs, const std::vector<RS_FLOAT>& numCoeffs)
{
	_order = denCoeffs.size();
	if (_order != numCoeffs.size())
	{
		throw std::logic_error("IIRFilter does not currently support mixed order filters");
	}
	_a = new RS_FLOAT[_order];
	_b = new RS_FLOAT[_order];
	_w = new RS_FLOAT[_order]();

	// TODO: can use std::copy
	for (unsigned i = 0; i < _order; i++)
	{
		_a[i] = denCoeffs[i];
		_b[i] = numCoeffs[i];
		_w[i] = 0;
	}
}

IirFilter::IirFilter(const RS_FLOAT* denCoeffs, const RS_FLOAT* numCoeffs, const unsigned order):
	_order(order)
{
	_a = new RS_FLOAT[order];
	_b = new RS_FLOAT[order];
	_w = new RS_FLOAT[order]();

	// TODO: can use std::copy
	for (unsigned i = 0; i < order; i++)
	{
		_a[i] = denCoeffs[i];
		_b[i] = numCoeffs[i];
		_w[i] = 0;
	}
}

IirFilter::~IirFilter()
{
	delete[] _a;
	delete[] _b;
	delete[] _w;
}

RS_FLOAT IirFilter::filter(const RS_FLOAT sample)
{
	// TODO: can use std::rotate
	for (unsigned j = _order - 1; j > 0; j--)
	{
		_w[j] = _w[j - 1];
	}
	_w[0] = sample;
	for (unsigned j = 1; j < _order; j++)
	{
		_w[0] -= _a[j] * _w[j];
	}
	RS_FLOAT out = 0;
	for (unsigned j = 0; j < _order; j++)
	{
		out += _b[j] * _w[j];
	}
	return out;
}

void IirFilter::filter(RS_FLOAT* samples, const int size)
{
	// TODO: can use std::rotate and std::inner_product
	for (int i = 0; i < size; i++)
	{
		for (unsigned j = _order - 1; j > 0; j--)
		{
			_w[j] = _w[j - 1];
		}
		_w[0] = samples[i];
		for (unsigned j = 1; j < _order; j++)
		{
			_w[0] -= _a[j] * _w[j];
		}
		samples[i] = 0;
		for (unsigned j = 0; j < _order; j++)
		{
			samples[i] += _b[j] * _w[j];
		}
	}
}

// =====================================================================================================================
//
// FIRFILTER CLASS
//
// =====================================================================================================================

FirFilter::FirFilter(const std::vector<RS_FLOAT>& coeffs)
{
	_order = coeffs.size();
	_filter = new RS_FLOAT[_order];
	_w = new RS_FLOAT[_order]();
	// TODO: can use std::copy
	for (unsigned i = 0; i < _order; i++)
	{
		_filter[i] = coeffs[i];
		_w[i] = 0;
	}
}

FirFilter::FirFilter(const RS_FLOAT* coeffs, const unsigned count) : _order(count)
{
	_filter = new RS_FLOAT[_order];
	_w = new RS_FLOAT[_order]();
	// TODO: can use std::copy
	for (unsigned i = 0; i < _order; i++)
	{
		_filter[i] = coeffs[i];
		_w[i] = 0;
	}
}

FirFilter::~FirFilter()
{
	delete[] _filter;
	delete[] _w;
}

inline RS_FLOAT FirFilter::filter(RS_FLOAT sample)
{
	return 0;
}

inline void FirFilter::filter(RS_FLOAT* samples, const int size)
{
	// See Oppenheim and Scaffer, section 6.5 "Basic Network Structures for FIR Systems"
	// TODO: Implement one of the more efficient FIR filter forms
	// TODO: Can use std::rotate
	auto* line = new RS_FLOAT[_order];
	std::memset(line, 0, sizeof(RS_FLOAT) * _order);
	for (int i = 0; i < size; i++)
	{
		line[0] = samples[i];
		RS_FLOAT res = 0;
		for (unsigned j = 0; j < _order; j++)
		{
			res += line[_order - j - 1] * _filter[j];
		}
		samples[i] = res;
		for (unsigned j = _order; j > 0; j--)
		{
			line[j] = line[j - 1];
		}
	}
	delete[] line;
}

inline void FirFilter::filter(std::complex<RS_FLOAT>* samples, const unsigned size) const
{
	// TODO: Can use std::rotate
	auto* line = new RS_COMPLEX[_order];
	for (unsigned i = 0; i < _order; i++)
	{
		line[i] = 0;
	}
	for (int i = 0; i < size; i++)
	{
		line[0] = samples[i];
		RS_COMPLEX res(0.0, 0.0);
		for (unsigned j = 0; j < _order; j++)
		{
			res += line[_order - j - 1] * _filter[j];
		}
		samples[i] = res;
		for (unsigned j = _order - 1; j > 0; j--)
		{
			line[j] = line[j - 1];
		}
	}
	delete[] line;
}

// =====================================================================================================================
//
// ARFILTER CLASS
//
// =====================================================================================================================

ArFilter::ArFilter(const std::vector<RS_FLOAT>& coeffs)
{
	// TODO: Can use std::copy
	_order = coeffs.size();
	_filter = new RS_FLOAT[_order];
	_w = new RS_FLOAT[_order]();

	for (unsigned i = 0; i < _order; i++)
	{
		_filter[i] = coeffs[i];
		_w[i] = 0;
	}
}

ArFilter::~ArFilter()
{
	delete[] _filter;
	delete[] _w;
}

RS_FLOAT ArFilter::filter(const RS_FLOAT sample)
{
	// TODO: Can use std::rotate
	for (unsigned j = _order - 1; j > 0; j--)
	{
		_w[j] = _w[j - 1];
	}
	_w[0] = sample;
	for (unsigned j = 1; j < _order; j++)
	{
		_w[0] -= _filter[j] * _w[j];
	}
	return _w[0];
}

void ArFilter::filter(RS_FLOAT* samples, const int size)
{
	for (int i = 0; i < size; i++)
	{
		// TODO: Can use std::rotate
		for (unsigned j = _order - 1; j > 0; j--)
		{
			_w[j] = _w[j - 1];
		}
		_w[0] = samples[i];
		for (unsigned j = 1; j < _order; j++)
		{
			_w[0] -= _filter[j] * _w[j];
		}
		samples[i] = _w[0];
	}
}

// =====================================================================================================================
//
// UPSAMPLER CLASS
//
// =====================================================================================================================

Upsampler::Upsampler(const int ratio):
	_ratio(ratio)
{
	_filter_size = 8 * ratio + 1;
	_filterbank = new RS_FLOAT[_filter_size];
	for (int i = 0; i < _filter_size; i++)
	{
		const RS_FLOAT window_value = 0.54 - 0.46 * std::cos(2 * M_PI * i / static_cast<RS_FLOAT>(_filter_size));
		const RS_FLOAT filter_value = sinc(1.0 / static_cast<RS_FLOAT>(ratio) * (i - _filter_size / 2.0));
		_filterbank[i] = filter_value * window_value;
	}
	_sample_memory = new RS_FLOAT[_filter_size / ratio + 1]();
}

Upsampler::~Upsampler()
{
	delete[] _filterbank;
	delete[] _sample_memory;
}

inline RS_FLOAT Upsampler::getSample(const RS_FLOAT* samples, const int n) const
{
	return n >= 0 ? samples[n] : _sample_memory[n + _filter_size];
}

void Upsampler::upsample(const RS_FLOAT* inSamples, const int inSize, RS_FLOAT* outSamples, const int outSize) const
{
	if (outSize != _ratio * inSize)
	{
		throw std::runtime_error("Target array size is not correct in Upsample");
	}
	// Polyphase upsampler implementation
	// See fers_upsample_p.m in the documentation for more details
	// Follows the diagram in section 4.7.4 "Polyphase Implementation of Interpolation Filters" of
	// Discrete Time Signal Processing, 2nd ed., Oppenheim and Schafer
	for (int i = 0, branch = 0; i < inSize; i++, branch++)
	{
		if (branch >= _ratio)
		{
			branch = 0;
		}
		outSamples[i] = 0;
		for (int j = branch; j < _filter_size; j += _ratio)
		{
			outSamples[i] += _filterbank[j] * getSample(inSamples, i - j / _ratio);
		}
	}
	// TODO: Can use std::copy
	if (const int transfer_size = _filter_size / _ratio + 1; inSize >= transfer_size)
	{
		memcpy(_sample_memory, &inSamples[inSize - transfer_size], transfer_size * sizeof(RS_FLOAT));
	}
	else
	{
		for (int i = 0; i < transfer_size - inSize; i++)
		{
			_sample_memory[i] = _sample_memory[i + inSize];
		}
		for (int i = 0; i < inSize; i++)
		{
			_sample_memory[i + transfer_size - inSize] = inSamples[i];
		}
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
	constexpr RS_FLOAT den_coeffs[12] = {
		1.0, -10.301102119865, 48.5214567642597, -137.934509572412, 262.914952985445,
		-352.788381841481, 340.027874008585, -235.39260470286, 114.698499845697,
		-37.4634653062448, 7.38208765922137, -0.664807695826097
	};
	constexpr RS_FLOAT num_coeffs[12] = {
		2.7301694322809e-06, -1.8508123430239e-05, 5.75739466753894e-05, -0.000104348734423658,
		0.000111949190289715, -4.9384188225528e-05, -4.9384188225522e-05, 0.00011194919028971,
		-0.000104348734423656, 5.75739466753884e-05, -1.85081234302388e-05, 2.73016943228086e-06
	};
	_filter = new IirFilter(den_coeffs, num_coeffs, 12);
}

DecadeUpsampler::~DecadeUpsampler()
{
	delete _filter;
}

void DecadeUpsampler::upsample(const RS_FLOAT sample, RS_FLOAT* out) const
{
	out[0] = sample;
	// TODO: Can use std::fill
	for (int i = 1; i < 10; i++)
	{
		out[i] = 0;
	}
	_filter->filter(out, 10);
}

void DecadeUpsampler::upsample(const RS_FLOAT* in, const int count, RS_FLOAT* out) const
{
	for (int i = 0; i < count; i++)
	{
		out[i * 10] = in[i];
		// TODO: Can use std::fill
		for (int j = 1; j < 10; j++)
		{
			out[i * 10 + j] = 0;
		}
	}
	_filter->filter(out, count * 10);
}
