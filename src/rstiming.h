//Timing source for simulation - all objects must be slaved to a timing source
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//16 October 2006

#ifndef RS_TIMING_H
#define RS_TIMING_H

#include <config.h>
#include <string>
#include <vector>
#include <boost/utility.hpp>

namespace rs
{
	//Forward definitions
	class ClockModelGenerator; //rstiming.h

	///Timing controls the timing of systems to which it is attached
	class Timing : boost::noncopyable
	{
	public:
		/// Constructor
		explicit Timing(const std::string& name);

		/// Destructor
		virtual ~Timing();

		/// Get the real time of a particular pulse
		virtual rsFloat getPulseTimeError() const = 0;

		/// Get the next sample of time error for a particular pulse
		virtual rsFloat nextNoiseSample() = 0;

		/// Skip a sample, computing only enough to preserve long term correlations
		virtual void skipSamples(long long samples) = 0;

		/// Get the name of the timing source
		std::string getName() const;

	private:
		std::string _name; //!< The name of the prototype this is based on
	};

	/// Prototypes for timing sources used by system devices
	class PrototypeTiming
	{
	public:
		/// Constructor
		explicit PrototypeTiming(const std::string& name);

		/// Add an alpha and a weight to the timing prototype
		void addAlpha(rsFloat alpha, rsFloat weight);

		/// Get the alphas and weights from the prototype
		void getAlphas(std::vector<rsFloat>& getAlphas, std::vector<rsFloat>& getWeights) const;

		/// Get the phase offset
		rsFloat getPhaseOffset() const;

		/// Get the frequency offset
		rsFloat getFreqOffset() const;

		/// Get the frequency
		rsFloat getFrequency() const;

		/// Get the value of the sync on pulse flag
		bool getSyncOnPulse() const;

		/// Set a constant frequency offset
		void addFreqOffset(rsFloat offset);

		/// Set a constant phase offset
		void addPhaseOffset(rsFloat offset);

		/// Set a random frequency offset
		void addRandomFreqOffset(rsFloat stdev);

		/// Set a random phase offset
		void addRandomPhaseOffset(rsFloat stdev);

		/// Set the base frequency of the clock model
		void setFrequency(rsFloat freq);

		/// Get the name of the prototype
		std::string getName() const;

		/// Set the sync on pulse flag -- timing error resets at the start of the pulse
		void setSyncOnPulse();

	private:
		std::string _name; //!< The name of the prototype timing source
		std::vector<rsFloat> _alphas; //!< Alpha parameters for 1/f^alpha clock model
		std::vector<rsFloat> _weights; //!< Weights for 1/f^alpha clock model
		rsFloat _freq_offset; //!< Constant frequency offset
		rsFloat _phase_offset; //!< Constant phase offset
		rsFloat _random_phase; //!< Standard deviation of random phase offset
		rsFloat _random_freq; //!< Standard deviation of random frequency offset
		rsFloat _frequency; //!< The nominal oscillator frequency
		bool _sync_on_pulse; //!< Reset timing error at the start of each pulse
	};

	/// Implementation of clock timing based on the 1/f model with linear interpolation
	class ClockModelTiming final : public Timing
	{
	public:
		/// Constructor
		explicit ClockModelTiming(const std::string& name);

		/// Destructor
		virtual ~ClockModelTiming();

		/// Get the next sample of time error for a particular pulse
		virtual rsFloat nextNoiseSample();

		/// Skip a sample, computing only enough to preserve long term correlations
		virtual void skipSamples(long long samples);

		/// Reset the clock phase error to zero
		void reset() const;

		/// Get the value of the sync on pulse flag
		bool getSyncOnPulse() const;

		/// Initialize the clock model generator
		void initializeModel(const PrototypeTiming* timing);

		/// Get the real time of a particular pulse
		virtual rsFloat getPulseTimeError() const;

		/// Get the carrier frequency of the modelled clock
		rsFloat getFrequency() const;

		/// Return the enabled state of the clock model
		bool enabled() const;

	private:
		bool _enabled; //!< Is the clock model going to produce non-zero samples?
		ClockModelGenerator* _model; //!< Clock model for intra-pulse samples
		std::vector<rsFloat> _alphas; //!< Alpha parameters for 1/f^alpha clock model
		std::vector<rsFloat> _weights; //!< Weights for 1/f^alpha clock model
		rsFloat _frequency{}; //!< Carrier frequency of the modelled clock
		bool _sync_on_pulse{}; //!< Reset the timing at the start of each pulse
	};
}

#endif
