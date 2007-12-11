//Timing source for simulation - all objects must be slaved to a timing source
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//16 October 2006

#ifndef __RS_TIMING_H
#define __RS_TIMING_H

#include <config.h>
#include <boost/utility.hpp>
#include <string>
#include <vector>
#include "rsnoise.h"

namespace rs {
  ///Timing controls the timing of systems to which it is attached
  class Timing : boost::noncopyable {
  public:
    /// Constructor
    Timing(const std::string &name);
    /// Destructor
    virtual ~Timing();
    /// Get the real time of a particular pulse
    virtual rsFloat GetPulseTimeError(int pulse) const = 0;
    /// Get the next sample of time error for a particular pulse
    virtual rsFloat NextNoiseSample() = 0;
    /// Advance to the next pulse
    virtual void PulseDone() = 0;
    /// Get the name of the timing source
    std::string GetName() const;
  private:
    std::string name; //!< The name of the prototype this is based on
  };

  /// Prototypes for timing sources used by system devices
  class PrototypeTiming {
  public:
    /// Constructor
    PrototypeTiming(const std::string &name);
    /// Add an alpha and a weight to the timing prototype
    void AddAlpha(rsFloat alpha, rsFloat weight);
    /// Get the alphas and weights from the prototype
    void GetAlphas(std::vector<rsFloat> &get_alphas, std::vector<rsFloat> &get_weights) const;
    /// Get the name of the prototype
    std::string GetName() const;
  private:
    std::string name; //!< The name of the prototype timing source
    std::vector<rsFloat> alphas; //!< Alpha parameters for 1/f^alpha clock model
    std::vector<rsFloat> weights; //!< Weights for 1/f^alpha clock model
  };

  /// Implementation of clock timing based on the 1/f model with linear interpolation
  class ClockModelTiming: public Timing {
  public:
    /// Constructor
    ClockModelTiming(const std::string &name, int num_pulses, int pulse_length, int pulse_gap);
    /// Destructor
    virtual ~ClockModelTiming();
    /// Get the real time of a particular pulse
    virtual rsFloat GetPulseTimeError(int pulse) const;
    /// Get the next sample of time error for a particular pulse
    virtual rsFloat NextNoiseSample();
    /// Advance to the next pulse
    virtual void PulseDone();
    /// Initialize the clock model generator
    virtual void InitializeModel(const PrototypeTiming *timing);
    /// Return the enabled state of the clock model
    bool Enabled();
  private:
    bool enabled; //!< Is the clock model going to produce non-zero samples?
    int num_pulses; //!< The number of pulses for which timing is required
    int pulse_length; //!< The length of each pulse, in samples
    int pulse_gap; //!< The length of the gap between pulses, in samples
    int current_pulse; //!< Pulse we are generating samples for
    ClockModelGenerator *model; //!< Clock model for intra-pulse samples
    rsFloat *trend_samples; //!< Pulse beginning and end samples
    std::vector<rsFloat> alphas; //!< Alpha parameters for 1/f^alpha clock model
    std::vector<rsFloat> weights; //!< Weights for 1/f^alpha clock model
  };

  /// Implementation of clock timing limited to intra-pulse times
  class PulseTiming: public Timing {
  public:
    /// Constructor
    PulseTiming(const std::string &name, int num_pulses);
    /// Destructor
    virtual ~PulseTiming();
    /// Get the real time of a particular pulse
    virtual rsFloat GetPulseTimeError(int pulse) const;
    /// Get the next sample of time error for a particular pulse
    virtual rsFloat NextNoiseSample();
    /// Advance to the next pulse
    virtual void PulseDone();
    /// Initialize the clock model generator
    virtual void InitializeModel(const PrototypeTiming *timing);
  private:
    int num_pulses; //!< The number of pulses for which timing is required
    rsFloat *trend_samples; //!< Pulse beginning and end samples
  };

}

#endif
