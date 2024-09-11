//rsnoise.cpp - Functions for generating different types of noise
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//14 August 2006

#include "rsnoise.h"

#include <cmath>
#include <limits>
#include <memory>
#include <boost/random.hpp>

#include "rsdebug.h"
#include "rsdsp.h"
#include "rsparameters.h"

using namespace rs;

namespace
{
	//Use the Mersenne Twister PRNG with parameter 19937, for why this is a good choice see
	//Mersenne Twister: A 623-dimensionally equidistributed uniform pseudo-random number generator
	//Matsumoto et al.
	//ACM Transactions on Modeling and Computer Simulation
	//January 1998
	boost::mt19937* rng;

	//Used to generate single noise samples
	boost::normal_distribution<> nd(0, 1);
	boost::uniform_real<> ud(0, 1.0);
	boost::variate_generator<boost::mt19937&, boost::normal_distribution<>>* normal_vg;
	boost::variate_generator<boost::mt19937&, boost::uniform_real<>>* uniform_vg;
}

//
// Implementation of non-class functions
//

/// Initialize the random number generator code (must be called once, after the loading of the script)
void rs_noise::initializeNoise()
{
	delete rng;
	delete normal_vg;
	rng = new boost::mt19937(RsParameters::randomSeed());
	normal_vg = new boost::variate_generator<boost::mt19937&, boost::normal_distribution<>>(*rng, nd);
	uniform_vg = new boost::variate_generator<boost::mt19937&, boost::uniform_real<>>(*rng, ud);
}

/// Clean up the noise code
void rs_noise::cleanUpNoise()
{
	delete rng;
	delete normal_vg;
	delete uniform_vg;
}

/// Return a single sample of white gaussian noise
RS_FLOAT rs_noise::wgnSample(const RS_FLOAT stddev)
{
	if (stddev > std::numeric_limits<RS_FLOAT>::epsilon())
	{
		return (*normal_vg)() * stddev;
	}
	return 0;
}

/// Return a single uniformly distributed sample in [0, 1]
RS_FLOAT rs_noise::uniformSample()
{
	return (*uniform_vg)();
}

/// Calculate noise amplitude from the temperature
RS_FLOAT rs_noise::noiseTemperatureToPower(const RS_FLOAT temperature, const RS_FLOAT bandwidth)
{
	return RsParameters::boltzmannK() * temperature * bandwidth; //See equations.tex
}

//
// NoiseGenerator Implementation
//

//Constructor
NoiseGenerator::NoiseGenerator() = default;

//Destructor
NoiseGenerator::~NoiseGenerator() = default;

//
// Gamma Generator
//

/// Constructor
GammaGenerator::GammaGenerator(const RS_FLOAT k):
	_dist(k),
	_gen(*rng, _dist)
{
}

/// Destructor
GammaGenerator::~GammaGenerator() = default;

/// Get a single random sample
RS_FLOAT GammaGenerator::getSample()
{
	return _gen();
}

/// Operator to get a random sample
RS_FLOAT GammaGenerator::operator()()
{
	return _gen();
}

//
// WGNGenerator Implementation
//

//Constructor
WgnGenerator::WgnGenerator(const RS_FLOAT stddev)
{
	_dist = boost::normal_distribution<>(0, stddev);
	_gen = new boost::variate_generator<boost::mt19937&, boost::normal_distribution<>>(*rng, _dist);
}

//Default constructor
WgnGenerator::WgnGenerator()
{
	_dist = boost::normal_distribution<>(0, 1);
	_gen = new boost::variate_generator<boost::mt19937&, boost::normal_distribution<>>(*rng, _dist);
}

// Destructor
WgnGenerator::~WgnGenerator()
{
	delete _gen;
}

// Get a sample from the rng
RS_FLOAT WgnGenerator::getSample()
{
	return (*_gen)();
}

//
// FAlphaBranch Implementation
//

/// Constructor
FAlphaBranch::FAlphaBranch(const RS_FLOAT ffrac, const unsigned int fint, FAlphaBranch* pre, const bool last):
	_pre(pre),
	_last(last),
	_ffrac(ffrac),
	_fint(fint)
{
	rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "[VV] Making branch ffrac=%f fint=%d\n", ffrac, fint);
	//Calculate scale for upsampling
	_upsample_scale = std::pow(10, ffrac + fint + 0.5);
	//Initialize the filters for shaping, highpass and upsampling
	init();
	// Create a buffer for ten samples
	_buffer = new RS_FLOAT[10];
	if (!last)
	{
		refill();
	}
}

/// Destructor
FAlphaBranch::~FAlphaBranch()
{
	delete _pre;
	clean();
}

///Initialize the branch filters
void FAlphaBranch::init()
{
	_shape_filter = nullptr;
	_integ_filter = nullptr;
	_highpass = nullptr;
	//Create the upsampler
	_upsampler = new DecadeUpsampler();
	if (_pre)
	{
		/// Numerator coefficients for elliptical highpass
		constexpr
			RS_FLOAT hp_num[12] = {
				3.817871081981451e-01,
				-4.093384095523618e+00,
				2.005300512623078e+01,
				-5.924672881811163e+01,
				1.172948159891025e+02,
				-1.633810410083022e+02,
				1.633810410083034e+02,
				-1.172948159891052e+02,
				5.924672881811390e+01,
				-2.005300512623186e+01,
				4.093384095523903e+00,
				-3.817871081981776e-01
			};
		/// Denominator coefficients for elliptical highpass
		constexpr
			RS_FLOAT hp_den[12] = {
				1.000000000000000e+00,
				-8.829695665523831e+00,
				3.583068809011030e+01,
				-8.811479652970442e+01,
				1.457874067329429e+02,
				-1.702715637111961e+02,
				1.431504350055831e+02,
				-8.656925883534657e+01,
				3.687395592491803e+01,
				-1.052413841411803e+01,
				1.808292123637038e+00,
				-1.412932578340511e-01
			};
		//Initialize the highpass filter
		_highpass = new IirFilter(hp_den, hp_num, 12);
	}
	// Initialize the shaping filter
	if (_ffrac == 0.5)
	{
		/// Numerator co-efficients for 1/f^0.5 rolloff
		constexpr
			RS_FLOAT sf_num[16] = {
				5.210373977738306e-03,
				-7.694671394585578e-03,
				1.635979377907092e-03,
				9.852449140857658e-05,
				-2.080553126780113e-03,
				4.088764157029523e-03,
				-1.549082440084623e-03,
				9.054734252370680e-04,
				-3.467369912368729e-04,
				4.516383087838856e-04,
				-1.063356106118517e-03,
				1.330008998057684e-04,
				6.556909567323943e-04,
				-4.839476350293955e-04,
				6.664936170526832e-05,
				1.528520559763056e-05
			};
		constexpr
			RS_FLOAT sf_den[16] = {
				1.000000000000000e+00,
				-2.065565041154101e+00,
				1.130909190864681e+00,
				-1.671244644503288e-01,
				-3.331474931013877e-01,
				9.952625337612708e-01,
				-7.123036343635182e-01,
				3.297062696290504e-01,
				-1.925691520710595e-01,
				1.301247006176314e-01,
				-2.702016290409912e-01,
				1.455380885858886e-01,
				1.091921868353888e-01,
				-1.524953111510459e-01,
				5.667716332023935e-02,
				-2.890314873767405e-03
			};
		//Gain of shaping filter
		_shape_gain = 5.210373977738306e-03;
		//Create the shaping filter
		_shape_filter = new IirFilter(sf_den, sf_num, 16);
	}
	else if (_ffrac == 0)
	{
		_shape_filter = nullptr;
	}
	else
	{
		rs_debug::printf(rs_debug::RS_CRITICAL, "[CRITICAL] Value of ffrac is %f\n", _ffrac);
		throw std::runtime_error("Fractional integrator values other than 0.5 not currently supported");
	}
	//Initialize the integration filters
	if (_fint > 0)
	{
		//Gain of integration filter
		_integ_gain = 1; //2.755923548698047e-05;
		if (_fint == 1)
		{
			constexpr
				RS_FLOAT i_den[2] = {1, -1};
			constexpr
				RS_FLOAT i_num[2] = {1, 0};
			_integ_filter = new IirFilter(i_den, i_num, 2);
		}
		if (_fint == 2)
		{
			constexpr
				RS_FLOAT i_den[3] = {1, -2, 1};
			constexpr
				RS_FLOAT i_num[3] = {1, 0, 0};
			_integ_filter = new IirFilter(i_den, i_num, 3);
		}
		if (_fint > 2)
		{
			throw std::runtime_error("Only alpha values between 2 and -2 are supported for noise generation");
		}
	}
	//Initialize the offset
	_offset_sample = 0;
	_got_offset = false;
	// Create a buffer for ten samples
	_buffer = new RS_FLOAT[10];
	if (!_last)
	{
		refill();
	}
	_pre_scale = 1;
}

/// Get a sample from the branch
RS_FLOAT FAlphaBranch::getSample()  // NOLINT(misc-no-recursion)
{
	if (!_last)
	{
		const RS_FLOAT ret = _buffer[_buffer_samples];
		_buffer_samples++;
		if (_buffer_samples == 10)
		{
			refill();
		}
		return ret;
	}
	return calcSample() + _offset_sample * _upsample_scale;
}

///Clean up the filters, etc
void FAlphaBranch::clean() const
{
	delete _highpass;
	delete[] _buffer;
	delete _integ_filter;
	delete _shape_filter;
	delete _upsampler;
}

/// Calculate a single sample
RS_FLOAT FAlphaBranch::calcSample()  // NOLINT(misc-no-recursion)
{
	RS_FLOAT sample = rs_noise::wgnSample(1);
	if (_shape_filter)
	{
		sample = _shape_filter->filter(sample) / _shape_gain;
	}
	if (_integ_filter)
	{
		sample = _integ_filter->filter(sample) / _integ_gain;
	}
	if (_pre)
	{
		// Apply highpass only if we have branches below us
		sample = _highpass->filter(sample);
		// If there is a branch below us, add a sample from that
		if (_got_offset)
		{
			sample += _pre->getSample() * _pre_scale - _offset_sample;
		}
		else
		{
			_got_offset = true;
			_offset_sample = _pre->getSample() * _pre_scale;
		}
	}
	return sample;
}

/// Refill the buffer
void FAlphaBranch::refill()  // NOLINT(misc-no-recursion)
{
	const RS_FLOAT sample = calcSample();
	// Pass the sample to the upsampler
	_upsampler->upsample(sample, _buffer);
	// Scale the buffer
	for (int i = 0; i < 10; i++)
	{
		_buffer[i] *= _upsample_scale;
		_buffer[i] += _offset_sample;
	}
	// Reset the sample counter
	_buffer_samples = 0;
}

/// Refill the buffer
void FAlphaBranch::flush(const RS_FLOAT scale = 1.0)
{
	clean();
	init();
	_pre_scale = scale;
	//  upsample_scale = 0;
}

//
// MultirateGenerator Implementation
//

/// Constructor
MultirateGenerator::MultirateGenerator(const RS_FLOAT alpha, const unsigned int branches)
{
	const RS_FLOAT beta = -(alpha - 2) / 2.0;
	//Calculate the integer and fractional parts of beta
	const int fint = static_cast<int>(std::floor(beta));
	const RS_FLOAT ffrac = fmod(beta, 1);
	//Build the multirate filter tree
	createTree(ffrac, fint, branches);
	_scale = 1.0 / std::pow(10.0, (-alpha + 2) * 2.0);
}

/// Destructor
MultirateGenerator::~MultirateGenerator()
{
	delete _topbranch;
}

/// Get a single noise sample
RS_FLOAT MultirateGenerator::getSample()
{
	return _topbranch->getSample() * _scale;
}

/// Skip a number of samples, preseverving correlations
void MultirateGenerator::skipSamples(long long samples) const
{
	// rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Skip branches %d\n", skip_branches);
	if (const int skip_branches = static_cast<int>(std::floor(std::log10(samples))) - 1; skip_branches > 0)
	{
		std::vector<FAlphaBranch*> flushbranches;
		FAlphaBranch* branch = _topbranch;
		for (int i = 0; i < skip_branches && branch != nullptr; i++)
		{
			flushbranches.push_back(branch);
			branch = branch->_pre;
		}
		if (branch)
		{
			//Now generate the samples of the lower branches
			samples /= static_cast<long long>(std::pow(10.0, skip_branches));
			// rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Skipping %ld samples in %ld branches \n", samples, skip_branches);
			for (int i = 0; i < samples; i++)
			{
				branch->getSample();
			}
		}
		// Flush the buffers of the upper branches
		const int size = static_cast<int>(flushbranches.size());
		flushbranches[size - 1]->flush(std::pow(10.0, skip_branches - 2.0));
		for (int i = size - 2; i >= 0; i--)
		{
			flushbranches[i]->flush();
		}
	}
	else
	{
		for (int i = 0; i < samples; i++)
		{
			_topbranch->getSample();
		}
	}
}

/// Create the branches of the filter structure tree
// The tree is stored as a linked list, which each link a branch
void MultirateGenerator::createTree(const RS_FLOAT falpha, const int fint, const unsigned int branches)
{
	if (branches == 0)
	{
		throw std::runtime_error("Cannot create multirate noise generator with zero branches");
	}
	// If ffrac and fint are both zero, we only need a single branch
	if (falpha == 0 && fint == 0)
	{
		_topbranch = new FAlphaBranch(0, 0, nullptr, true);
	}
	else
	{
		_topbranch = nullptr;
		for (unsigned int i = 0; i < branches - 1; i++)
		{
			_topbranch = new FAlphaBranch(falpha, fint, _topbranch, false);
		}
		_topbranch = new FAlphaBranch(falpha, fint, _topbranch, true);
	}
}

/// Reset the output to zero
void MultirateGenerator::reset() const
{
	std::vector<FAlphaBranch*> flush_branches;
	//Build a reverse order list of branches
	FAlphaBranch* branch = _topbranch;
	while (branch)
	{
		flush_branches.push_back(branch);
		branch = branch->_pre;
	}
	// Flush the branch buffers in reverse order
	const int size = static_cast<int>(flush_branches.size());
	for (int i = size - 1; i >= 0; i--)
	{
		flush_branches[i]->flush();
	}
}

//
// ClockModelGenerator Implementation
//

/// Constructor
ClockModelGenerator::ClockModelGenerator(const std::vector<RS_FLOAT>& alpha, const std::vector<RS_FLOAT>& inWeights,
                                         const RS_FLOAT frequency, const RS_FLOAT phaseOffset, const RS_FLOAT freqOffset,
                                         const int branches):
	_phase_offset(phaseOffset),
	_freq_offset(freqOffset),
	_frequency(frequency)
{
	_weights = inWeights;
	auto iter = alpha.begin();
	auto witer = _weights.begin();
	// Create the generators for each band
	for (; iter != alpha.end(); ++iter, ++witer)
	{
		// MultirateGenerator* mgen = new MultirateGenerator(*iter, branches);
		// _generators.push_back(mgen);
		auto mgen = std::make_unique<MultirateGenerator>(*iter, branches);
		_generators.push_back(std::move(mgen));
		//Calibrate the weights using the measured calibration numbers
		if (*iter == 2)
		{
			*witer *= std::pow(10.0, 1.2250);
		}
		else if (*iter == 1)
		{
			*witer *= std::pow(10.0, 0.25);
		}
		else if (*iter == 0)
		{
			*witer *= std::pow(10.0, -0.25);
		}
		else if (*iter == -1)
		{
			*witer *= std::pow(10.0, -0.5);
		}
		else if (*iter == -2)
		{
			*witer *= std::pow(10.0, -1);
		}
	}
	_count = 0;
}

/// Destructor
ClockModelGenerator::~ClockModelGenerator() = default;

/// Get a single noise sample
RS_FLOAT ClockModelGenerator::getSample()
{
	RS_FLOAT sample = 0;
	// Get noise from the multirate generators for each band
	const int size = static_cast<int>(_generators.size());
	for (int i = 0; i < size; i++)
	{
		sample += _generators[i]->getSample() * _weights[i];
	}
	// Add the phase and frequency offsets
	sample += _phase_offset;
	//Calculate the count in clock frequencies
	sample += 2 * M_PI * _freq_offset * static_cast<double>(_count) / RsParameters::rate();
	_count++;
	return sample;
}

/// Skip some noise samples, calculating only the branches required to preserve correlations
void ClockModelGenerator::skipSamples(const long long samples)
{
	const int gens = static_cast<int>(_generators.size());
	for (int i = 0; i < gens; i++)
	{
		_generators[i]->skipSamples(samples);
	}
	_count += samples;
}

/// Reset the noise to zero
void ClockModelGenerator::reset()
{
	const int gens = static_cast<int>(_generators.size());
	for (int i = 0; i < gens; i++)
	{
		_generators[i]->reset();
	}
	_count = 0;
}

/// Is the generator going to produce non-zero samples
bool ClockModelGenerator::enabled() const
{
	if (!_generators.empty() || _freq_offset != 0 || _phase_offset != 0)
	{
		return true;
	}
	return false;
}

//
//PythonNoiseGenerator Implementation
//

///Constructor
PythonNoiseGenerator::PythonNoiseGenerator(const std::string& module, const std::string& function):
	_generator(module, function)
{
}

///Destructor
PythonNoiseGenerator::~PythonNoiseGenerator() = default;

///Get a single noise sample
RS_FLOAT PythonNoiseGenerator::getSample()
{
	return _generator.getSample();
}

