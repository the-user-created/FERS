//rstarget.h
//Defines the class for targets
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 26 April 2006
#ifndef __RSTARGET_H
#define __RSTARGET_H

#include <config.h>
#include "rsobject.h"
#include "rspath.h"

namespace rs {

  /// Target models a simple point target with a specified RCS pattern
  class Target: public Object {
  public:
    enum RCSPattern { RCS_ISOTROPIC, RCS_COSINE, RCS_ARBITRARY };
    /// Constructor
    Target(Platform *platform, std::string name);
    /// Destructor
    ~Target();
    /// Stores the gain pattern of the target
    RCSPattern pattern;
    /// Returns the Radar Cross Section at a particular angle
    rsFloat GetRCS(SVec3 &inAngle, SVec3 &outAngle) const;
    /// Set the global RCS factor
    void SetRCS(rsFloat newRCS, RCSPattern newPattern);
  private:
    rsFloat rcs; //!< RCS for Isotropic and cosine rcs patterns
    /// Calculate a cosine RCS pattern
    rsFloat CalcCosRCS(SVec3 &inAngle, SVec3 &outAngle) const;
    /// Calculate RCS based on an arbitrary pattern
    rsFloat CalcPatternRCS(SVec3 &inAngle, SVec3 &outAngle) const;
  };
}

#endif
