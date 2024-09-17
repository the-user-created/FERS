// noise_generators.cpp
// Functions for generating different types of noise
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 14 August 2006

#include "noise_generators.h"

#include <cmath>
#include <limits>
#include <memory>
#include <random>
#include <ranges>

#include "core/logging.h"
#include "core/parameters.h"
#include "math_utils/dsp_filters.h"

using namespace rs;

namespace
{
	// Use the Mersenne Twister PRNG with parameter 19937 from the C++ standard library
	std::unique_ptr<std::mt19937> rng;
	std::unique_ptr<std::normal_distribution<>> normal_dist;
	std::unique_ptr<std::uniform_real_distribution<>> uniform_dist;

	// Seed generation can be updated with a random device if no explicit seed is provided
	unsigned int getSeed()
	{
		return parameters::randomSeed(); // Assuming this function still provides a seed, else use random_device
	}
}

// =====================================================================================================================
//
// IMPLEMENTATIONS OF NON-CLASS FUNCTIONS
//
// =====================================================================================================================

void rs_noise::initializeNoise()
{
	// Initialize random number generator and distributions using smart pointers
	rng = std::make_unique<std::mt19937>(getSeed());
	normal_dist = std::make_unique<std::normal_distribution<>>(0.0, 1.0);
	uniform_dist = std::make_unique<std::uniform_real_distribution<>>(0.0, 1.0);
}

void rs_noise::cleanUpNoise()
{
	// Smart pointers automatically clean up, no need for manual deletion
	rng.reset();
	normal_dist.reset();
	uniform_dist.reset();
}

RS_FLOAT rs_noise::wgnSample(const double stddev)
{
	// Generate a white Gaussian noise sample with the specified standard deviation
	return stddev > std::numeric_limits<double>::epsilon() ? (*normal_dist)(*rng) * stddev : 0.0;
}

RS_FLOAT rs_noise::uniformSample()
{
	// Generate a sample from a uniform distribution between 0 and 1
	return (*uniform_dist)(*rng);
}

RS_FLOAT rs_noise::noiseTemperatureToPower(const double temperature, const double bandwidth)
{
	// Compute noise power given temperature and bandwidth
	return parameters::boltzmannK() * temperature * bandwidth;
}

// =====================================================================================================================
//
// GAMMA GENERATOR CLASS
//
// =====================================================================================================================

GammaGenerator::GammaGenerator(const RS_FLOAT k) : _rng(std::make_unique<std::mt19937>(parameters::randomSeed())),
                                                   _dist(k, 1.0) {}

// =====================================================================================================================
//
// WGN GENERATOR CLASS
//
// =====================================================================================================================

WgnGenerator::WgnGenerator(const RS_FLOAT stddev) : _rng(std::make_unique<std::mt19937>(parameters::randomSeed())),
                                                    _dist(0.0, stddev), _stddev(stddev) {}

WgnGenerator::WgnGenerator() : _rng(std::make_unique<std::mt19937>(parameters::randomSeed())), _dist(0.0, 1.0),
                               _stddev(1.0) {}

// =====================================================================================================================
//
// FALPHA BRANCH CLASS
//
// =====================================================================================================================

FAlphaBranch::FAlphaBranch(const RS_FLOAT ffrac, const unsigned fint, std::unique_ptr<FAlphaBranch> pre,
                           const bool last) : _pre(std::move(pre)), _last(last), _ffrac(ffrac), _fint(fint)
{
	logging::printf(logging::RS_VERY_VERBOSE, "[VV] Making branch ffrac=%f fint=%d\n", ffrac, fint);
	_upsample_scale = std::pow(10, ffrac + fint + 0.5);
	init();
	_buffer = std::make_unique<RS_FLOAT[]>(10);
	if (!last) { refill(); }
}

void FAlphaBranch::init()
{
	_upsampler = std::make_unique<DecadeUpsampler>();

	if (_pre)
	{
		constexpr std::array hp_num = {
			3.817871081981451e-01, -4.093384095523618e+00, 2.005300512623078e+01, -5.924672881811163e+01,
			1.172948159891025e+02, -1.633810410083022e+02, 1.633810410083034e+02, -1.172948159891052e+02,
			5.924672881811390e+01, -2.005300512623186e+01, 4.093384095523903e+00, -3.817871081981776e-01
		};
		constexpr std::array hp_den = {
			1.000000000000000e+00, -8.829695665523831e+00, 3.583068809011030e+01, -8.811479652970442e+01,
			1.457874067329429e+02, -1.702715637111961e+02, 1.431504350055831e+02, -8.656925883534657e+01,
			3.687395592491803e+01, -1.052413841411803e+01, 1.808292123637038e+00, -1.412932578340511e-01
		};
		_highpass = std::make_unique<IirFilter>(hp_den.data(), hp_num.data(), hp_num.size());
	}

	if (_ffrac == 0.5f)
	{
		constexpr std::array sf_num = {
			5.210373977738306e-03, -7.694671394585578e-03, 1.635979377907092e-03, 9.852449140857658e-05,
			-2.080553126780113e-03, 4.088764157029523e-03, -1.549082440084623e-03, 9.054734252370680e-04,
			-3.467369912368729e-04, 4.516383087838856e-04, -1.063356106118517e-03, 1.330008998057684e-04,
			6.556909567323943e-04, -4.839476350293955e-04, 6.664936170526832e-05, 1.528520559763056e-05
		};
		constexpr std::array sf_den = {
			1.000000000000000e+00, -2.065565041154101e+00, 1.130909190864681e+00, -1.671244644503288e-01,
			-3.331474931013877e-01, 9.952625337612708e-01, -7.123036343635182e-01, 3.297062696290504e-01,
			-1.925691520710595e-01, 1.301247006176314e-01, -2.702016290409912e-01, 1.455380885858886e-01,
			1.091921868353888e-01, -1.524953111510459e-01, 5.667716332023935e-02, -2.890314873767405e-03
		};
		_shape_gain = 5.210373977738306e-03;
		_shape_filter = std::make_unique<IirFilter>(sf_den.data(), sf_num.data(), sf_num.size());
	}
	else if (_ffrac != 0.0f)
	{
		logging::printf(logging::RS_CRITICAL, "[CRITICAL] Value of ffrac is %f\n", _ffrac);
		throw std::runtime_error("Fractional integrator values other than 0.5 are not supported");
	}

	if (_fint > 0)
	{
		_integ_gain = 1.0f;

		if (_fint == 1)
		{
			constexpr std::array<RS_FLOAT, 2> i_den = {1.0f, -1.0f};
			constexpr std::array<RS_FLOAT, 2> i_num = {1.0f, 0.0f};
			_integ_filter = std::make_unique<IirFilter>(i_den.data(), i_num.data(), i_num.size());
		}
		else if (_fint == 2)
		{
			constexpr std::array<RS_FLOAT, 3> i_den = {1.0f, -2.0f, 1.0f};
			constexpr std::array<RS_FLOAT, 3> i_num = {1.0f, 0.0f, 0.0f};
			_integ_filter = std::make_unique<IirFilter>(i_den.data(), i_num.data(), i_num.size());
		}
		else { throw std::runtime_error("Only alpha values between 2 and -2 are supported for noise generation"); }
	}

	_offset_sample = 0.0f;
	_got_offset = false;
	_buffer = std::make_unique<RS_FLOAT[]>(10);

	if (!_last) { refill(); }

	_pre_scale = 1.0f;
}

RS_FLOAT FAlphaBranch::getSample() // NOLINT(misc-no-recursion)
{
	if (!_last)
	{
		const RS_FLOAT ret = _buffer[_buffer_samples++];
		if (_buffer_samples == 10) { refill(); }
		return ret;
	}
	return calcSample() + _offset_sample * _upsample_scale;
}

void FAlphaBranch::clean()
{
	_highpass.reset();
	_integ_filter.reset();
	_shape_filter.reset();
	_upsampler.reset();
}

RS_FLOAT FAlphaBranch::calcSample() // NOLINT(misc-no-recursion)
{
	RS_FLOAT sample = rs_noise::wgnSample(1);

	if (_shape_filter) { sample = _shape_filter->filter(sample) / _shape_gain; }

	if (_integ_filter) { sample = _integ_filter->filter(sample) / _integ_gain; }

	if (_pre)
	{
		sample = _highpass->filter(sample);
		if (_got_offset) { sample += _pre->getSample() * _pre_scale - _offset_sample; }
		else
		{
			_got_offset = true;
			_offset_sample = _pre->getSample() * _pre_scale;
		}
	}

	return sample;
}

void FAlphaBranch::refill() // NOLINT(misc-no-recursion)
{
	const RS_FLOAT sample = calcSample();
	_upsampler->upsample(sample, _buffer.get());

	for (int i = 0; i < 10; ++i)
	{
		_buffer[i] *= _upsample_scale;
		_buffer[i] += _offset_sample;
	}

	_buffer_samples = 0;
}

void FAlphaBranch::flush(const RS_FLOAT scale = 1.0)
{
	clean();
	init();
	_pre_scale = scale;
}

// =====================================================================================================================
//
// MULTI-RATE GENERATOR CLASS
//
// =====================================================================================================================

MultirateGenerator::MultirateGenerator(const RS_FLOAT alpha, const unsigned branches)
{
	const RS_FLOAT beta = -(alpha - 2) / 2.0;
	const int fint = static_cast<int>(std::floor(beta));
	const RS_FLOAT ffrac = fmod(beta, 1.0);

	createTree(ffrac, fint, branches);

	_scale = 1.0 / std::pow(10.0, (-alpha + 2.0) * 2.0);
}


void MultirateGenerator::skipSamples(long long samples) const
{
	if (const int skip_branches = static_cast<int>(std::floor(std::log10(samples))) - 1; skip_branches > 0)
	{
		std::vector<FAlphaBranch*> flushbranches;
		const FAlphaBranch* branch = _topbranch.get(); // Using unique_ptr's get() to access raw pointer

		// Traverse the tree up to skip_branches or until the branch becomes null
		for (int i = 0; i < skip_branches && branch != nullptr; ++i)
		{
			flushbranches.push_back(const_cast<FAlphaBranch*>(branch));
			// Use const_cast to store non-const pointers for flushing
			branch = branch->getPre();
		}

		// If there is still a branch after skipping, call getSample the required number of times
		if (branch)
		{
			samples /= static_cast<long long>(std::pow(10.0, skip_branches));
			for (int i = 0; i < samples; ++i)
			{
				const_cast<FAlphaBranch*>(branch)->getSample(); // Remove constness for calling getSample
			}
		}

		// Flush the branches in reverse order to reset the chain
		const int size = static_cast<int>(flushbranches.size());
		if (size > 0) { flushbranches[size - 1]->flush(std::pow(10.0, skip_branches - 2.0)); }

		for (int i = size - 2; i >= 0; --i) { flushbranches[i]->flush(); }
	}
	else
	{
		// No branch skipping required, just generate samples from the top branch
		for (int i = 0; i < samples; ++i) { _topbranch->getSample(); }
	}
}

void MultirateGenerator::createTree(const RS_FLOAT fAlpha, const int fInt, const unsigned branches)
{
	if (branches == 0) { throw std::runtime_error("Cannot create multirate noise generator with zero branches"); }

	if (fAlpha == 0.0 && fInt == 0) { _topbranch = std::make_unique<FAlphaBranch>(0, 0, nullptr, true); }
	else
	{
		std::unique_ptr<FAlphaBranch> previous_branch = nullptr;

		// Build the chain of branches
		for (unsigned i = 0; i < branches - 1; ++i)
		{
			// Move the previous branch into the new FAlphaBranch constructor
			previous_branch = std::make_unique<FAlphaBranch>(fAlpha, fInt, std::move(previous_branch), false);
		}

		// The top branch is the last branch in the chain (set `last` to true)
		_topbranch = std::make_unique<FAlphaBranch>(fAlpha, fInt, std::move(previous_branch), true);
	}
}

void MultirateGenerator::reset() const
{
	std::vector<FAlphaBranch*> flush_branches;
	FAlphaBranch* branch = _topbranch.get(); // Use get() to access the raw pointer from unique_ptr

	// Collect all branches
	while (branch)
	{
		flush_branches.push_back(branch);
		branch = branch->getPre();
	}

	// Flush branches in reverse order
	for (const auto& flush_branche : std::ranges::reverse_view(flush_branches)) { flush_branche->flush(); }
}


// =====================================================================================================================
//
// CLOCK MODEL GENERATOR CLASS
//
// =====================================================================================================================


ClockModelGenerator::ClockModelGenerator(const std::vector<RS_FLOAT>& alpha, const std::vector<RS_FLOAT>& inWeights,
                                         const RS_FLOAT frequency, const RS_FLOAT phaseOffset,
                                         const RS_FLOAT freqOffset, int branches)
	: _weights(inWeights), _phase_offset(phaseOffset), _freq_offset(freqOffset), _frequency(frequency)
{
	auto iter = alpha.begin();
	auto witer = _weights.begin();

	for (; iter != alpha.end(); ++iter, ++witer)
	{
		auto mgen = std::make_unique<MultirateGenerator>(*iter, branches);
		_generators.push_back(std::move(mgen));

		switch (static_cast<int>(*iter))
		{
		case 2: *witer *= std::pow(10.0, 1.2250);
			break;
		case 1: *witer *= std::pow(10.0, 0.25);
			break;
		case 0: *witer *= std::pow(10.0, -0.25);
			break;
		case -1: *witer *= std::pow(10.0, -0.5);
			break;
		case -2: *witer *= std::pow(10.0, -1);
			break;
		default: *witer *= 1.0; // No change to the weight
			break;
		}
	}
}

RS_FLOAT ClockModelGenerator::getSample()
{
	RS_FLOAT sample = 0;

	for (size_t i = 0; i < _generators.size(); ++i) { sample += _generators[i]->getSample() * _weights[i]; }

	sample += _phase_offset;
	sample += 2 * M_PI * _freq_offset * static_cast<double>(_count) / parameters::rate();
	++_count;

	return sample;
}

void ClockModelGenerator::skipSamples(const long long samples)
{
	for (const auto& generator : _generators) { generator->skipSamples(samples); }

	_count += samples;
}

void ClockModelGenerator::reset()
{
	for (const auto& generator : _generators) { generator->reset(); }

	_count = 0;
}
