//rsnoise.h Functions and classes to generate noise of various types
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//14 August 2006

#ifndef RS_NOISE_H
#define RS_NOISE_H

#include <config.h>
#include <memory>
#include <vector>
#include <boost/utility.hpp>
#include <boost/random/gamma_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/variate_generator.hpp>

#include "rsdsp.h"
#include "rspython.h"


namespace rs_noise
{
	/// Initialize the random number generator code (must be called once, after the loading of the script)
	void initializeNoise();

	/// Clean up the noise code
	void cleanUpNoise();

	/// Return a single sample of white gaussian noise
	rsFloat wgnSample(rsFloat stddev);

	/// Return a single uniformly distributed sample in [0, 1]
	rsFloat uniformSample();

	/// Calculate noise power from the temperature
	rsFloat noiseTemperatureToPower(rsFloat temperature, rsFloat bandwidth);
}

namespace rs
{
	/// NoiseGenerator - base class for different types of noise generator
	class NoiseGenerator : boost::noncopyable
	{
	public:
		/// Constructor
		NoiseGenerator();

		/// Destructor
		virtual ~NoiseGenerator();

		/// Get a single random sample
		virtual rsFloat getSample() = 0;
	};

	/// Generator of Gaussian white noise
	class WgnGenerator final : public NoiseGenerator
	{
	public:
		/// Constructor
		explicit WgnGenerator(rsFloat stddev);

		/// Default Constructor
		WgnGenerator();

		/// Destructor
		virtual ~WgnGenerator();

		/// Get a single random sample
		virtual rsFloat getSample();

	private:
		boost::normal_distribution<rsFloat> _dist; //!< PRNG distribution
		boost::variate_generator<boost::mt19937&, boost::normal_distribution<>>* _gen;
		//!< Variate Generator (see boost::random docs)
		rsFloat _temperature{}; //!< Noise temperature
	};

	/// Gamma distributed noise generator
	class GammaGenerator final : public NoiseGenerator
	{
	public:
		/// Constructor
		explicit GammaGenerator(rsFloat k); //x_bar is the 'scale' parameter and k is the 'shape' parameter
		/// Destructor
		virtual ~GammaGenerator();

		/// Get a single random sample
		virtual rsFloat getSample();

		/// Operator to get a random sample
		rsFloat operator()();

	private:
		boost::gamma_distribution<rsFloat> _dist; //!< Gamma distribution function
		boost::variate_generator<boost::mt19937&, boost::gamma_distribution<>> _gen; //!< Variate generator
	};


	/// Single branch of the multirate clock generator model
	class FAlphaBranch : boost::noncopyable
	{
	public:
		/// Constructor
		FAlphaBranch(rsFloat ffrac, unsigned int fint, FAlphaBranch* pre, bool last);

		/// Destructor
		~FAlphaBranch();

		/// Get a sample from the branch
		rsFloat getSample();

		/// Flush the buffer, and refull with samples
		void flush(rsFloat scale);

	private:
		/// Initialize the filters, etc.
		void init();

		/// Clean up the filters
		void clean() const;

		/// Refill the buffer with samples
		void refill();

		/// Calculate a single sample
		rsFloat calcSample();

		IirFilter* _shape_filter; //!< The filter for shaping the noise
		rsFloat _shape_gain; //!< Gain of shaping filter
		IirFilter* _integ_filter; //!< Integrator filter
		rsFloat _integ_gain; //!< Gain of integration filter

		rsFloat _upsample_scale; //!< Scaling factor for upsampling
		IirFilter* _highpass; //!< Highpass filter
		FAlphaBranch* _pre; //!< Next lower branch in the structure
		bool _last; //!< If this filter is the top branch, don't upsample
		DecadeUpsampler* _upsampler; //!< Upsampler for this branch
		rsFloat* _buffer; //!< Buffer for storing samples from the upsampler
		unsigned int _buffer_samples; //!< Number of samples available in the buffer
		rsFloat _ffrac; //!< Fractional part of filter curve
		rsFloat _fint; //!< Integer part of filter curve
		rsFloat _offset_sample; //!< Sample from the branch below us
		bool _got_offset; //!< Are we waiting for the offset
		rsFloat _pre_scale; //!< Previous branch scale factor
		friend class MultirateGenerator;
	};


	/// Class to generate 1/f^alpha noise based on multirate approach
	class MultirateGenerator final : public NoiseGenerator
	{
	public:
		/// Constructor
		MultirateGenerator(rsFloat alpha, unsigned int branches);

		/// Destructor
		~MultirateGenerator();

		/// Get a single noise sample
		rsFloat getSample();

		/// Skip a number of samples, preserving correlations of period longer than the sample count
		void skipSamples(long long samples) const;

		/// Reset the output to zero
		void reset() const;

	private:
		rsFloat _scale; //!< Scale for normalizing values
		/// Create the branches of the filter structure tree
		void createTree(rsFloat falpha, int fint, unsigned int branches);

		/// Get the co-efficients of the shaping filter
		FAlphaBranch* _topbranch; //!< Top branch of the filter structure tree
	};

	/// Class to generate noise based on the weighted sum of 1/f^alpha noise
	class ClockModelGenerator final : public NoiseGenerator
	{
	public:
		/// Constructor
		ClockModelGenerator(const std::vector<rsFloat>& alpha, const std::vector<rsFloat>& inWeights,
		                    rsFloat frequency, rsFloat phaseOffset, rsFloat freqOffset, int branches);

		/// Destructor
		~ClockModelGenerator();

		/// Get a single noise sample
		rsFloat getSample();

		/// Skip noise samples, calculating only enough to preserve long-term correlations
		void skipSamples(long long samples);

		/// Reset the noise to zero
		void reset();

		/// Is the generator going to produce non-zero samples
		bool enabled() const;

	private:
		std::vector<std::unique_ptr<MultirateGenerator>> _generators; // The multirate generators which generate noise in each band
		std::vector<rsFloat> _weights; // Weight of the noise from each generator
		rsFloat _phase_offset; //!< Offset from nominal phase
		rsFloat _freq_offset; //!< Offset from nominal base frequency
		rsFloat _frequency; //!< Nominal base frequency
		unsigned long _count; //!< Number of samples generated by this generator
	};

	/// Generator of noise using a python module
	class PythonNoiseGenerator final : public NoiseGenerator
	{
	public:
		///Constructor
		PythonNoiseGenerator(const std::string& module, const std::string& function);

		///Destructor
		~PythonNoiseGenerator();

		///Get a single noise sample
		rsFloat getSample();

	private:
		rs_python::PythonNoise _generator;
	};
}

#endif
