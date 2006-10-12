//rsantenna.h
//Class for Antennas, with different gain patterns, etc
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//20 July 2006

#ifndef __RSANTENNA_H
#define __RSANTENNA_H

#include <config.h>
#include <string>
#include "rsgeometry.h"
#include "boost/utility.hpp"

namespace rs {
  //Forward declaration of SVec3 and Vec3 (see rspath.h)
  class SVec3;
  class Vec3;

  /// The antenna class defines an antenna, which may be used by one or more transmitters
  class Antenna: public boost::noncopyable { //Antennas are not meant to be copied
  public:
    /// Default constructor
    Antenna(std::string name);
    /// Destructor
    virtual ~Antenna();
    /// Returns the current gain at a particular angle
    virtual rsFloat GetGain(const SVec3& angle, const SVec3& refangle, rsFloat wavelength) const = 0;
    /// Returns the noise temperature at a particular angle
    virtual rsFloat GetNoiseTemperature(const SVec3& angle) const;
    /// Set the antenna's loss factor (values > 1 are physically impossible)
    void SetEfficiencyFactor(rsFloat loss);
    /// Gets the Loss Factor
    rsFloat GetEfficiencyFactor() const;
    /// Return the name of the antenna
    std::string GetName() const;
  protected:
    /// Get the angle off boresight
    rsFloat GetAngle(const SVec3 &angle, const SVec3 &refangle) const;
  private:
    /// The loss factor for this antenna
    rsFloat lossFactor; //!< Loss factor
    /// The name of the antenna
    std::string name;
  };

}

namespace rsAntenna {

  //Antenna with an Isotropic radiation pattern
  class Isotropic: public rs::Antenna {
  public:
    /// Default constructor
    Isotropic(std::string name);
    /// Default destructor
    ~Isotropic();
    /// Get the gain at an angle
    rsFloat GetGain(const rs::SVec3 &angle, const rs::SVec3 &refangle, rsFloat wavelength) const;
  };

  //Antenna with a sinc (sinx/x) radiation pattern
  class Sinc: public rs::Antenna {
  public:
    /// Constructor
    Sinc(std::string name, rsFloat alpha, rsFloat beta, rsFloat gamma);
    /// Default destructor
    ~Sinc();
    /// Get the gain at an angle
    rsFloat GetGain(const rs::SVec3 &angle, const rs::SVec3 &refangle, rsFloat wavelength) const;
  private:
    rsFloat alpha; //!< First parameter (see equations.tex)
    rsFloat beta; //!< Second parameter (see equations.tex)
    rsFloat gamma; //!< Third parameter (see equations.tex)
  };

  /// Square horn antenna
  class SquareHorn: public rs::Antenna {
  public:
    /// Constructor
    SquareHorn(std::string name, rsFloat dimension);
    /// Default destructor
    ~SquareHorn();
    /// Get the gain at an angle
    rsFloat GetGain(const rs::SVec3 &angle, const rs::SVec3 &refangle, rsFloat wavelength) const;
  private:
    rsFloat dimension; //!< The linear size of the horn
  };

  /// Parabolic dish antenna
  class ParabolicReflector: public rs::Antenna {
  public:
    /// Constructor
    ParabolicReflector(std::string name, rsFloat diameter);
    /// Default destructor
    ~ParabolicReflector();
    /// Get the gain at an angle
    rsFloat GetGain(const rs::SVec3 &angle, const rs::SVec3 &refangle, rsFloat wavelength) const;
  private:
    rsFloat diameter;    
  };
}

#endif
