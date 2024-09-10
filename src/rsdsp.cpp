//Digital Signal Processing support functions
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//30 July 2007

#include "rsdsp.h"

#include <cmath>
#include <cstring>
#include <stdexcept>

#include "rsdebug.h"
#include "rsparameters.h"

using namespace rs;

//
// Support Functions
//
namespace
{
	//Calculate sin(pi * x) / (pi*x)
	RS_FLOAT sinc(const RS_FLOAT x)
	{
		if (x == 0)
		{
			return 1.0;
		}
		return std::sin(x * M_PI) / (x * M_PI);
	}

	/// Create a FIR filter using the Blackman window
	RS_FLOAT* blackmanFir(const RS_FLOAT cutoff, int& length)
	{
		// Use double the render filter length, for faster rolloff than the render filter
		length = RsParameters::renderFilterLength() * 2;
		RS_FLOAT* coeffs = new RS_FLOAT[length];
		const RS_FLOAT n = length / 2.0;
		for (int i = 0; i < length; i++)
		{
			const RS_FLOAT filt = sinc(cutoff * (i - n));
			// We use the Blackman window, for a suitable tradeoff between rolloff and stopband attenuation
			// Equivalent Kaiser beta = 7.04 (Oppenhiem and Schaffer, Hamming)
			const RS_FLOAT window = 0.42 - 0.5 * cos(M_PI * i / n) + 0.08 * cos(2 * M_PI * i / n);
			coeffs[i] = filt * window;
		}
		return coeffs;
	}
}

/// Upsample size samples stored *in by an integer ratio and store the result in (pre-allocated) out
// TODO: this would be better as a multirate upsampler
// In fact, the whole scheme is currently sub-optimal - we could use better filters, better windows and a better approach
// it works ok for now, users seeking higher accuracy can oversample outside FERS until this is fixed
void rs::upsample(const RsComplex* in, const int size, RsComplex* out, const int ratio)
{
	/// Design the FIR filter for de-imaging
	int filt_length;
	const RS_FLOAT* coeffs = blackmanFir(1 / static_cast<RS_FLOAT>(ratio), filt_length);

	// Temporary buffer for zero padding and results
	RsComplex* tmp = new RsComplex[size * ratio + filt_length];
	for (int i = 0; i < size * ratio + filt_length; i++)
	{
		tmp[i] = 0;
	}
	/// Stuff the data with a suitable number of zeros
	for (int i = 0; i < size; i++)
	{
		tmp[i * ratio] = in[i];
		for (int j = 1; j < ratio - 1; j++)
		{
			tmp[i * ratio + j] = 0;
		}
	}
	// Create a FIR filter object
	const FirFilter filt(coeffs, filt_length);
	filt.filter(tmp, size * ratio + filt_length);
	// Copy results to output buffer
	for (int i = 0; i < size * ratio; i++)
	{
		out[i] = tmp[i + filt_length / 2 - 1];
	}
	// Clean up
	delete[] tmp;
	delete[] coeffs;
}

/// Upsample size samples stored *in by an integer ratio and store the result in (pre-allocated) out
// TODO: This would be better (and much faster) as a multirate downsampler
void rs::downsample(const RsComplex* in, const int size, RsComplex* out, const int ratio)
{
	/// Design the FIR filter for anti-aliasing
	int filt_length;
	const RS_FLOAT* coeffs = blackmanFir(1 / static_cast<RS_FLOAT>(ratio), filt_length);
	// Temporary buffer for zero padding and results
	RsComplex* tmp = new RsComplex[size + filt_length];
	for (int i = size - 1; i < size + filt_length; i++)
	{
		tmp[i] = 0;
	}
	// Copy the input into the temporary buffer, leaving zero padding at the end
	for (int i = 0; i < size; i++)
	{
		tmp[i] = in[i];
	}
	// Run the anti aliasing filter on the data
	const FirFilter filt(coeffs, filt_length);
	filt.filter(tmp, size + filt_length);

	//Copy the results to the output buffer
	for (int i = 0; i < size / ratio; i++)
	{
		out[i] = tmp[i * ratio + filt_length / 2] / static_cast<RS_FLOAT>(ratio);
		//    printf("%f+%fi\n", out[i].real(), out[i].imag());
	}
	// Clean up
	delete[] coeffs;
	delete[] tmp;
}

//
// Filter Implementation
//

/// Constructor
DspFilter::DspFilter()
{
}

/// Destructor
DspFilter::~DspFilter()
{
}

//
// IIRFilter Implementation
//

/// Constructor
IirFilter::IirFilter(const std::vector<RS_FLOAT>& denCoeffs, const std::vector<RS_FLOAT>& numCoeffs)
{
	//Get the filter order
	_order = denCoeffs.size();
	//Check the filter order
	if (_order != numCoeffs.size())
	{
		throw std::logic_error("IIRFilter does not currently support mixed order filters");
	}
	//Allocate memory to store co-efficients and state
	_a = new RS_FLOAT[_order];
	_b = new RS_FLOAT[_order];
	_w = new RS_FLOAT[_order];
	//Load the co-efficients from the vectors into the arrays
	for (unsigned int i = 0; i < _order; i++)
	{
		_a[i] = denCoeffs[i];
		_b[i] = numCoeffs[i];
		_w[i] = 0;
	}
}

/// Constructor
IirFilter::IirFilter(const RS_FLOAT* denCoeffs, const RS_FLOAT* numCoeffs, const unsigned int order):
	_order(order)
{
	_a = new RS_FLOAT[order];
	_b = new RS_FLOAT[order];
	_w = new RS_FLOAT[order];
	// Load the coefficients into the arrays
	for (unsigned int i = 0; i < order; i++)
	{
		_a[i] = denCoeffs[i];
		_b[i] = numCoeffs[i];
		_w[i] = 0;
	}
}

/// Destructor
IirFilter::~IirFilter()
{
	//Clean up the co-efficients and state
	delete[] _a;
	delete[] _b;
	delete[] _w;
}

/// Pass a single sample through the filter
RS_FLOAT IirFilter::filter(const RS_FLOAT sample)
{
	//Shift the filter state
	for (unsigned int j = _order - 1; j > 0; j--)
	{
		_w[j] = _w[j - 1];
	}
	// Calculate w[0]
	_w[0] = sample;
	for (unsigned int j = 1; j < _order; j++)
	{
		_w[0] -= _a[j] * _w[j];
	}
	//Calculate y[n]
	RS_FLOAT out = 0;
	for (unsigned int j = 0; j < _order; j++)
	{
		out += _b[j] * _w[j];
	}
	return out;
}

/// Pass an array of samples through the filter, filtering in place
void IirFilter::filter(RS_FLOAT* samples, const int size)
{
	for (int i = 0; i < size; i++)
	{
		//Shift the filter state
		for (unsigned int j = _order - 1; j > 0; j--)
		{
			_w[j] = _w[j - 1];
		}
		// Calculate w[0]
		_w[0] = samples[i];
		for (unsigned int j = 1; j < _order; j++)
		{
			_w[0] -= _a[j] * _w[j];
		}
		//Calculate y[n]
		samples[i] = 0;
		for (unsigned int j = 0; j < _order; j++)
		{
			samples[i] += _b[j] * _w[j];
		}
	}
}

//
// FIRFilter implementation
//

/// Constructor
FirFilter::FirFilter(const std::vector<RS_FLOAT>& coeffs)
{
	//Get the filter order
	_order = coeffs.size();
	//Allocate memory to store co-efficients and state
	_filter = new RS_FLOAT[_order];
	_w = new RS_FLOAT[_order];
	//Load the co-efficients from the vectors into the arrays
	for (unsigned int i = 0; i < _order; i++)
	{
		_filter[i] = coeffs[i];
		_w[i] = 0;
	}
}

/// Constructor from coeffs
FirFilter::FirFilter(const RS_FLOAT* coeffs, const int count)
{
	_order = count;
	// Allocate memory to store co-efficients and state
	_filter = new RS_FLOAT[_order];
	_w = new RS_FLOAT[_order];
	// Load the co-efficients
	for (unsigned int i = 0; i < _order; i++)
	{
		_filter[i] = coeffs[i];
		_w[i] = 0;
	}
}

/// Destructor
FirFilter::~FirFilter()
{
	// Clean up memory
	delete[] _filter;
	delete[] _w;
}

/// Pass a single sample through the filter
inline RS_FLOAT FirFilter::filter(RS_FLOAT sample)
{
	return 0;
}

/// Pass an array of samples through the filter, filtering in place
// See Oppenheim and Scaffer, section 6.5 "Basic Network Structures for FIR Systems"
// TODO: Implement one of the more efficient FIR filter forms
inline void FirFilter::filter(RS_FLOAT* samples, const int size)
{
	// Allocate memory for a delay line, equal to the filter length
	RS_FLOAT* line = new RS_FLOAT[_order];
	std::memset(line, 0, sizeof(RS_FLOAT) * _order);
	// Perform the inplace convolution with the pulse
	for (int i = 0; i < size; i++)
	{
		line[0] = samples[i];
		RS_FLOAT res = 0;
		for (unsigned int j = 0; j < _order; j++)
		{
			res += line[_order - j - 1] * _filter[j];
		}
		samples[i] = res;
		//Move the line over by one sample
		for (int j = _order; j > 0; j--)
		{
			line[j] = line[j - 1];
		}
	}
	//Clean up
	delete[] line;
}

/// Pass an array of complex samples through the filter, filtering in place
inline void FirFilter::filter(std::complex<RS_FLOAT>* samples, const int size) const
{
	// Allocate memory for a delay line, equal to the filter length
	RsComplex* line = new RsComplex[_order];
	for (unsigned int i = 0; i < _order; i++)
	{
		line[i] = 0;
	}
	// Perform the inplace convolution with the pulse
	for (int i = 0; i < size; i++)
	{
		line[0] = samples[i];
		RsComplex res(0.0, 0.0);
		for (unsigned int j = 0; j < _order; j++)
		{
			res += line[_order - j - 1] * _filter[j];
		}
		samples[i] = res;
		//Move the line over by one sample
		for (int j = _order - 1; j > 0; j--)
		{
			line[j] = line[j - 1];
		}
	}
	//Clean up
	delete[] line;
}

//
// ARFilter implementation
//


/// Constructor
ArFilter::ArFilter(const std::vector<RS_FLOAT>& coeffs)
{
	//Get the filter order
	_order = coeffs.size();
	//Allocate memory to store co-efficients and state
	_filter = new RS_FLOAT[_order];
	_w = new RS_FLOAT[_order];
	//Load the co-efficients from the vectors into the arrays
	for (unsigned int i = 0; i < _order; i++)
	{
		_filter[i] = coeffs[i];
		_w[i] = 0;
	}
}

/// Destructor
ArFilter::~ArFilter()
{
	// Clean up memory
	delete[] _filter;
	delete[] _w;
}

/// Pass a single sample through the filter
RS_FLOAT ArFilter::filter(const RS_FLOAT sample)
{
	//Shift the filter state
	for (unsigned int j = _order - 1; j > 0; j--)
	{
		_w[j] = _w[j - 1];
	}
	// Calculate w[0]
	_w[0] = sample;
	for (unsigned int j = 1; j < _order; j++)
	{
		_w[0] -= _filter[j] * _w[j];
	}
	//Return the output value of the filter
	return _w[0];
}

/// Pass an array of samples through the filter, filtering in place
void ArFilter::filter(RS_FLOAT* samples, const int size)
{
	for (int i = 0; i < size; i++)
	{
		//Shift the filter state
		for (unsigned int j = _order - 1; j > 0; j--)
		{
			_w[j] = _w[j - 1];
		}
		// Calculate w[0]
		_w[0] = samples[i];
		for (unsigned int j = 1; j < _order; j++)
		{
			_w[0] -= _filter[j] * _w[j];
		}
		//Calculate y[n]
		samples[i] = _w[0];
	}
}

//
// Upsampler implementation
//

/// Constructor
Upsampler::Upsampler(const int ratio):
	_ratio(ratio)
{
	//Create the FIR interpolation filter
	_filter_size = 8 * ratio + 1; // 8*ratio should give adequate performance
	//Allocate memory for the filter bank
	_filterbank = new RS_FLOAT[_filter_size];
	// Simple windowed sinc filter design procedure
	for (int i = 0; i < _filter_size; i++)
	{
		// The Hamming window provides a solid tradeoff between rolloff and stopband attenuation
		const RS_FLOAT window_value = 0.54 - 0.46 * std::cos(2 * M_PI * i / static_cast<RS_FLOAT>(_filter_size));
		const RS_FLOAT filter_value = sinc(1.0 / static_cast<RS_FLOAT>(ratio) * (i - _filter_size / 2));
		_filterbank[i] = filter_value * window_value;
	}
	//Allocate memory for the sample state
	_sample_memory = new RS_FLOAT[_filter_size / ratio + 1];
	//Clear sample memory to zero
	for (int i = 0; i < _filter_size / ratio + 1; i++)
	{
		_sample_memory[i] = 0;
	}
}

/// Destructor
Upsampler::~Upsampler()
{
	// Clean up filter and state
	delete[] _filterbank;
	delete[] _sample_memory;
}

//Get a sample, from either the provided pointer or sample memory
inline RS_FLOAT Upsampler::getSample(const RS_FLOAT* samples, const int n) const
{
	if (n >= 0)
	{
		return samples[n];
	}
	return _sample_memory[n + _filter_size];
}

/// Upsamples a signal and applies an anti-imaging filter
void Upsampler::upsample(const RS_FLOAT* inSamples, const int inSize, RS_FLOAT* outSamples, const int outSize) const
{
	//Check the target array size
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
		// Apply the branch filter to the data
		for (int j = branch; j < _filter_size; j += _ratio)
		{
			outSamples[i] += _filterbank[j] * getSample(inSamples, i - j / _ratio);
		}
	}
	//Update the sample history
	if (const int transfer_size = _filter_size / _ratio + 1; inSize >= transfer_size)
	{
		memcpy(_sample_memory, &inSamples[inSize - transfer_size], transfer_size * sizeof(RS_FLOAT));
	}
	else
	{
		// Shift existing samples
		for (int i = 0; i < transfer_size - inSize; i++)
		{
			_sample_memory[i] = _sample_memory[i + inSize];
		}
		// Add new samples to the end of the buffer
		for (int i = 0; i < inSize; i++)
		{
			_sample_memory[i + transfer_size - inSize] = inSamples[i];
		}
	}
}

//
// DecadeUpsample Implementation
//

/// Constructor
DecadeUpsampler::DecadeUpsampler()
{
	/// 11th order elliptic lowpass at 0.1fs
	constexpr
	RS_FLOAT den_coeffs[12] = {
		1.0,
		-10.301102119865,
		48.5214567642597,
		-137.934509572412,
		262.914952985445,
		-352.788381841481,
		340.027874008585,
		-235.39260470286,
		114.698499845697,
		-37.4634653062448,
		7.38208765922137,
		-0.664807695826097
	};

	constexpr
	RS_FLOAT num_coeffs[12] = {
		2.7301694322809e-06,
		-1.8508123430239e-05,
		5.75739466753894e-05,
		-0.000104348734423658,
		0.000111949190289715,
		-4.9384188225528e-05,
		-4.9384188225522e-05,
		0.00011194919028971,
		-0.000104348734423656,
		5.75739466753884e-05,
		-1.85081234302388e-05,
		2.73016943228086e-06
	};
	//Initialize the anti-imaging filter
	_filter = new IirFilter(den_coeffs, num_coeffs, 12);
}

/// Destructor
DecadeUpsampler::~DecadeUpsampler()
{
	delete _filter;
}


///Upsample a single sample at a time
void DecadeUpsampler::upsample(const RS_FLOAT sample, RS_FLOAT* out) const
{
	// Prepare the output array
	out[0] = sample;
	for (int i = 1; i < 10; i++)
	{
		out[i] = 0;
	}
	// Filter in place
	_filter->filter(out, 10);
}

// Upsample a whole batch of samples
void DecadeUpsampler::upsample(const RS_FLOAT* in, const int count, RS_FLOAT* out) const
{
	/// Prepare the array for filtering
	for (int i = 0; i < count; i++)
	{
		out[i * 10] = in[i];
		for (int j = 1; j < 10; j++)
		{
			out[i * 10 + j] = 0;
		}
	}
	/// Filter in place
	_filter->filter(out, count * 10);
}

