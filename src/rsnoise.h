//rsnoise.h Functions and classes to generate noise of various types
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//14 August 2006

#ifndef RS_NOISE_H
#define RS_NOISE_H

#include <memory>
#include <vector>
#include <boost/utility.hpp>
#include <boost/random/gamma_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/variate_generator.hpp>

#include "config.h"
#include "rsdsp.h"
#include "rspython.h"


namespace rs_noise
{
	/// Initialize the random number generator code (must be called once, after the loading of the script)
	void initializeNoise();

	/// Clean up the noise code
	void cleanUpNoise();

	/// Return a single sample of white gaussian noise
	RS_FLOAT wgnSample(RS_FLOAT stddev);

	/// Return a single uniformly distributed sample in [0, 1]
	RS_FLOAT uniformSample();

	/// Calculate noise power from the temperature
	RS_FLOAT noiseTemperatureToPower(RS_FLOAT temperature, RS_FLOAT bandwidth);
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
		virtual RS_FLOAT getSample() = 0;
	};

	/// Generator of Gaussian white noise
	class WgnGenerator final : public NoiseGenerator
	{
	public:
		/// Constructor
		explicit WgnGenerator(RS_FLOAT stddev);

		/// Default Constructor
		WgnGenerator();

		/// Destructor
		~WgnGenerator() override;

		/// Get a single random sample
		RS_FLOAT getSample() override;

	private:
		boost::normal_distribution<> _dist; //!< PRNG distribution
		boost::variate_generator<boost::mt19937&, boost::normal_distribution<>>* _gen;
		//!< Variate Generator (see boost::random docs)
		RS_FLOAT _temperature{}; //!< Noise temperature
	};

	/// Gamma distributed noise generator
	class GammaGenerator final : public NoiseGenerator
	{
	public:
		/// Constructor
		explicit GammaGenerator(RS_FLOAT k); //x_bar is the 'scale' parameter and k is the 'shape' parameter
		/// Destructor
		~GammaGenerator() override;

		/// Get a single random sample
		RS_FLOAT getSample() override;

		/// Operator to get a random sample
		RS_FLOAT operator()();

	private:
		boost::gamma_distribution<> _dist; //!< Gamma distribution function
		boost::variate_generator<boost::mt19937&, boost::gamma_distribution<>> _gen; //!< Variate generator
	};


	/// Single branch of the multirate clock generator model
	class FAlphaBranch : boost::noncopyable
	{
	public:
		/// Constructor
		FAlphaBranch(RS_FLOAT ffrac, unsigned int fint, FAlphaBranch* pre, bool last);

		/// Destructor
		~FAlphaBranch();

		/// Get a sample from the branch
		RS_FLOAT getSample();

		/// Flush the buffer, and refull with samples
		void flush(RS_FLOAT scale);

	private:
		/// Initialize the filters, etc.
		void init();

		/// Clean up the filters
		void clean() const;

		/// Refill the buffer with samples
		void refill();

		/// Calculate a single sample
		RS_FLOAT calcSample();

		IirFilter* _shape_filter{}; //!< The filter for shaping the noise
		RS_FLOAT _shape_gain{}; //!< Gain of shaping filter
		IirFilter* _integ_filter{}; //!< Integrator filter
		RS_FLOAT _integ_gain{}; //!< Gain of integration filter

		RS_FLOAT _upsample_scale; //!< Scaling factor for upsampling
		IirFilter* _highpass{}; //!< Highpass filter
		FAlphaBranch* _pre; //!< Next lower branch in the structure
		bool _last; //!< If this filter is the top branch, don't upsample
		DecadeUpsampler* _upsampler{}; //!< Upsampler for this branch
		RS_FLOAT* _buffer; //!< Buffer for storing samples from the upsampler
		unsigned int _buffer_samples{}; //!< Number of samples available in the buffer
		RS_FLOAT _ffrac; //!< Fractional part of filter curve
		RS_FLOAT _fint; //!< Integer part of filter curve
		RS_FLOAT _offset_sample{}; //!< Sample from the branch below us
		bool _got_offset{}; //!< Are we waiting for the offset
		RS_FLOAT _pre_scale{}; //!< Previous branch scale factor
		friend class MultirateGenerator;
	};


	/// Class to generate 1/f^alpha noise based on multirate approach
	class MultirateGenerator final : public NoiseGenerator
	{
	public:
		/// Constructor
		MultirateGenerator(RS_FLOAT alpha, unsigned int branches);

		/// Destructor
		~MultirateGenerator() override;

		/// Get a single noise sample
		RS_FLOAT getSample() override;

		/// Skip a number of samples, preserving correlations of period longer than the sample count
		void skipSamples(long long samples) const;

		/// Reset the output to zero
		void reset() const;

	private:
		RS_FLOAT _scale; //!< Scale for normalizing values
		/// Create the branches of the filter structure tree
		void createTree(RS_FLOAT falpha, int fint, unsigned int branches);

		/// Get the co-efficients of the shaping filter
		FAlphaBranch* _topbranch{}; //!< Top branch of the filter structure tree
	};

	/// Class to generate noise based on the weighted sum of 1/f^alpha noise
	class ClockModelGenerator final : public NoiseGenerator
	{
	public:
		/// Constructor
		ClockModelGenerator(const std::vector<RS_FLOAT>& alpha, const std::vector<RS_FLOAT>& inWeights,
		                    RS_FLOAT frequency, RS_FLOAT phaseOffset, RS_FLOAT freqOffset, int branches);

		/// Destructor
		~ClockModelGenerator() override;

		/// Get a single noise sample
		RS_FLOAT getSample() override;

		/// Skip noise samples, calculating only enough to preserve long-term correlations
		void skipSamples(long long samples);

		/// Reset the noise to zero
		void reset();

		/// Is the generator going to produce non-zero samples
		[[nodiscard]] bool enabled() const;

	private:
		std::vector<std::unique_ptr<MultirateGenerator>> _generators; // The multirate generators which generate noise in each band
		std::vector<RS_FLOAT> _weights; // Weight of the noise from each generator
		RS_FLOAT _phase_offset; //!< Offset from nominal phase
		RS_FLOAT _freq_offset; //!< Offset from nominal base frequency
		RS_FLOAT _frequency; //!< Nominal base frequency
		unsigned long _count; //!< Number of samples generated by this generator
	};

	/// Generator of noise using a python module
	class PythonNoiseGenerator final : public NoiseGenerator
	{
	public:
		///Constructor
		PythonNoiseGenerator(const std::string& module, const std::string& function);

		///Destructor
		~PythonNoiseGenerator() override;

		///Get a single noise sample
		RS_FLOAT getSample() override;

	private:
		rs_python::PythonNoise _generator;
	};
}

#endif
