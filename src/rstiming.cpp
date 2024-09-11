//rstiming.cpp - Implementation of timing sources

#include "rstiming.h"

#include <utility>

#include "rsdebug.h"
#include "rsnoise.h"

using namespace rs;

//
// Timing Implementation
//

/// Default constructor
Timing::Timing(std::string  name):
	_name(std::move(name))
{
}

/// Destructor
Timing::~Timing() = default;

/// Get the name of the timing source
std::string Timing::getName() const
{
	return _name;
}

//
// PrototypeTiming Implementation
//

/// Constructor
PrototypeTiming::PrototypeTiming(std::string  name):
	_name(std::move(name))
{
	_freq_offset = 0;
	_phase_offset = 0;
	_random_phase = 0;
	_random_freq = 0;
	_frequency = 0;
	_sync_on_pulse = false;
}

/// Add an alpha and a weight to the timing prototype
void PrototypeTiming::addAlpha(const RS_FLOAT alpha, const RS_FLOAT weight)
{
	_alphas.push_back(alpha);
	_weights.push_back(weight);
}

/// Get the alphas and weights from the prototype
void PrototypeTiming::getAlphas(std::vector<RS_FLOAT>& getAlphas, std::vector<RS_FLOAT>& getWeights) const
{
	//Copy the alpha and weight vectors
	getAlphas = _alphas;
	getWeights = _weights;
}

/// Set a constant frequency offset
void PrototypeTiming::addFreqOffset(const RS_FLOAT offset)
{
	if (_random_freq != 0.0)
	{
		rs_debug::printf(rs_debug::RS_IMPORTANT,
		                "[Important] Random frequency offset and constant frequency offset are set for timing source %s. Only the random offset will be used.",
		                getName().c_str());
	}
	_freq_offset = offset;
}

/// Set a constant phase offset
void PrototypeTiming::addPhaseOffset(const RS_FLOAT offset)
{
	if (_random_phase != 0.0)
	{
		rs_debug::printf(rs_debug::RS_IMPORTANT,
		                "[Important] Random phase offset and constant phase offset are set for timing source %s. Only the random offset will be used.",
		                getName().c_str());
	}
	_phase_offset = offset;
}

/// Set a random frequency offset
void PrototypeTiming::addRandomFreqOffset(const RS_FLOAT stdev)
{
	if (_freq_offset != 0.0)
	{
		rs_debug::printf(rs_debug::RS_IMPORTANT,
		                "[Important] Random frequency offset and constant frequency offset are set for timing source %s. Only the random offset will be used.",
		                getName().c_str());
	}
	_random_freq = stdev;
}

/// Set a random phase offset
void PrototypeTiming::addRandomPhaseOffset(const RS_FLOAT stdev)
{
	if (_phase_offset != 0.0)
	{
		rs_debug::printf(rs_debug::RS_IMPORTANT,
		                "[Important] Random phase offset and constant phase offset are set for timing source %s. Only the random offset will be used.",
		                getName().c_str());
	}
	_random_phase = stdev;
}

/// Get the phase offset
RS_FLOAT PrototypeTiming::getPhaseOffset() const
{
	if (_random_phase != 0)
	{
		return rs_noise::wgnSample(_random_phase);
	}
	return _phase_offset;
}

/// Get the phase offset
RS_FLOAT PrototypeTiming::getFreqOffset() const
{
	if (_random_freq != 0)
	{
		return rs_noise::wgnSample(_random_freq);
	}
	return _freq_offset;
}

/// Get the frequency
RS_FLOAT PrototypeTiming::getFrequency() const
{
	return _frequency;
}


/// Get the name of the prototype
std::string PrototypeTiming::getName() const
{
	return _name;
}

/// Set the base frequency of the clock model
void PrototypeTiming::setFrequency(const RS_FLOAT freq)
{
	_frequency = freq;
}

/// Set the sync on pulse flag -- timing error resets at the start of the pulse
void PrototypeTiming::setSyncOnPulse()
{
	_sync_on_pulse = true;
}

/// Get the value of the sync on pulse flag
bool PrototypeTiming::getSyncOnPulse() const
{
	return _sync_on_pulse;
}

//
// ClockModelTiming Implementation
//

/// Constructor
ClockModelTiming::ClockModelTiming(const std::string& name):
	Timing(name),
	_enabled(false),
	_model(nullptr)
{
}

/// Destructor
ClockModelTiming::~ClockModelTiming()
{
	delete _model;
}

/// Initialize the clock model generator
void ClockModelTiming::initializeModel(const PrototypeTiming* timing)
{
	if (!_alphas.empty())
	{
		throw std::logic_error("[BUG] ClockModelTiming::InitializeModel called more than once");
	}
	//Copy the alpha and weight vectors
	timing->getAlphas(_alphas, _weights);
	rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "%d\n", _alphas.size());
	//Create the generator
	_model = new ClockModelGenerator(_alphas, _weights, timing->getFrequency(), timing->getPhaseOffset(),
	                                timing->getFreqOffset(), 15);
	//Warn if frequency is not set
	if (timing->getFrequency() == 0.0)
	{
		rs_debug::printf(rs_debug::RS_IMPORTANT,
		                "[Important] Timing source frequency not set, results could be incorrect.");
	}
	//Get the carrier frequency
	_frequency = timing->getFrequency();
	// Get the sync on pulse flag
	_sync_on_pulse = timing->getSyncOnPulse();
	//Enable the model
	_enabled = true;
}

/// Return the enabled state of the clock model
bool ClockModelTiming::enabled() const
{
	return _enabled && _model->enabled();
}

/// Get the real time of a particular pulse
RS_FLOAT ClockModelTiming::getPulseTimeError() const
{
	if (_enabled)
	{
		return _model->getSample();
	}
	return 0;
}

/// Skip a sample, computing only enough to preserve long term correlations
void ClockModelTiming::skipSamples(const long long samples)
{
	if (_enabled)
	{
		_model->skipSamples(samples);
	}
}

/// Get the value of the sync on pulse flag
bool ClockModelTiming::getSyncOnPulse() const
{
	return _sync_on_pulse;
}

/// Reset the clock phase error to zero
void ClockModelTiming::reset() const
{
	_model->reset();
}

/// Get the next sample of time error for a particular pulse
RS_FLOAT ClockModelTiming::nextNoiseSample()
{
	if (_enabled)
	{
		return _model->getSample();
	}
	return 0;
}

/// Get the carrier frequency of the modelled clock
RS_FLOAT ClockModelTiming::getFrequency() const
{
	return _frequency;
}
