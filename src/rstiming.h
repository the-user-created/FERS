//Timing source for simulation - all objects must be slaved to a timing source
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//16 October 2006

#ifndef __RS_TIMING_H
#define __RS_TIMING_H

#include <config.h>
#include <boost/utility.hpp>
#include <string>

namespace rs {
  ///Timing controls the timing of systems to which it is attached
  class Timing : boost::noncopyable {
  public:
    /// Default constructor
    Timing(const std::string &name, rsFloat rate, rsFloat jitter);
    /// Destructor
    ~Timing();
    /// Get the nearest pulse to a specified time
    rsFloat GetTime(rsFloat approx_time, rsFloat propagation_error) const;
    /// Get the name of the timing source
    std::string GetName() const;
  private:
    std::string name; //!< The name of the timing source
    rsFloat rate; //!< The rate of the clock (Hz)
    rsFloat jitter; //!< RMS time interval error
  };

}

#endif
