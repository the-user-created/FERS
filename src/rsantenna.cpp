//rsantenna.cpp
//Implementation of Antenna Class
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//20 July 2006

#include "rsantenna.h"
#include "rsdebug.h"
#include <cmath>
#include "rsportable.h"

using namespace rs;
using namespace rsAntenna;

namespace {
  //Return sin(x)/x
  rsFloat sinc(rsFloat theta)
  {
    return std::sin(theta)/(theta+std::numeric_limits<rsFloat>::epsilon());
  }

  //Return the first order, first kind bessel function of x, divided by x
  rsFloat j1c(rsFloat x)
  {
    if (x == 0)
      return 1;
    return rsPortable::BesselJ1(x)/(x);
  }
}

//Default constructor for the antenna
Antenna::Antenna(std::string name):
  lossFactor(1), //Antenna efficiency default is unity
  name(name)
{
}

//Antenna destructor
Antenna::~Antenna()
{
}

//Set the efficiency factor of the antenna
void Antenna::SetEfficiencyFactor(rsFloat loss)
{
  if (loss > 1)
    DEBUG_PRINT(rsDebug::RS_IMPORTANT, "Using greater than unity antenna efficiency, results might be inconsistent with reality");
  lossFactor = loss;
}

//Get the efficiency factor
rsFloat Antenna::GetEfficiencyFactor() const
{
  return lossFactor;
}

//Return the name of the antenna
std::string Antenna::GetName() const
{
  return name;
}

// Get the angle (in radians) off boresight
rsFloat Antenna::GetAngle(const SVec3 &angle, const SVec3 &refangle) const
{
  //Get the angle off boresight
  SVec3 normangle(angle);
  normangle.length = 1;
  Vec3 cangle(normangle);
  Vec3 ref(refangle);
  return std::acos(DotProduct(cangle, ref));
}

/// Get the noise temperature of the antenna in a particular direction
rsFloat Antenna::GetNoiseTemperature(const SVec3 &angle) const
{
  return 0; //TODO: Antenna noise temperature calculation
}

//
//Isotropic Implementation
//

//Default constructor
Isotropic::Isotropic(std::string name):
  Antenna(name)
{
}

//Default destructor
Isotropic::~Isotropic()
{
}

//Return the gain of the antenna
rsFloat Isotropic::GetGain(const SVec3 &angle, const SVec3 &refangle, rsFloat wavelength) const
{
  return GetEfficiencyFactor();
}

//
//Sinc Implemetation
//

//Constructor
Sinc::Sinc(std::string name, rsFloat alpha, rsFloat beta, rsFloat gamma):
  Antenna(name),
  alpha(alpha),
  beta(beta),
  gamma(gamma)
{
}

//Default destructor
Sinc::~Sinc()
{
}

// Return the gain of the antenna at an angle
rsFloat Sinc::GetGain(const SVec3 &angle, const SVec3 &refangle, rsFloat wavelength) const
{
  //Get the angle off boresight
  rsFloat theta = GetAngle(angle, refangle);
  //See "Sinc Pattern" in doc/equations.tex for equation used here
  rsFloat gain = alpha*std::pow(::sinc(beta*theta), gamma);
  return gain*GetEfficiencyFactor();
}

//
// SquareHorn Implementation
//

//Constructor
SquareHorn::SquareHorn(std::string name, rsFloat dimension):
  Antenna(name),
  dimension(dimension)
{
}

//Destructor
SquareHorn::~SquareHorn()
{
}

//Return the gain of the antenna
//See doc/equations.tex for details
rsFloat SquareHorn::GetGain(const SVec3 &angle, const SVec3 &refangle, rsFloat wavelength) const
{
  rsFloat Ge = 4*M_PI*dimension*dimension/(wavelength*wavelength);
  rsFloat x = M_PI*dimension*std::sin(GetAngle(angle, refangle))/wavelength;
  rsFloat gain = Ge*std::pow(::sinc(x), 2);
  return gain*GetEfficiencyFactor();
}


//
// Parabolic Dish Antenna
//

// Constructor
ParabolicReflector::ParabolicReflector(std::string name, rsFloat diameter):
  Antenna(name),
  diameter(diameter)
{
}

//Destructor
ParabolicReflector::~ParabolicReflector()
{
}

//Return the gain of the antenna
//See doc/equations.tex for details
rsFloat ParabolicReflector::GetGain(const SVec3 &angle, const SVec3 &refangle, rsFloat wavelength) const
{
  rsFloat Ge = std::pow(M_PI*diameter/wavelength, 2);
  rsFloat x = M_PI*diameter*std::sin(GetAngle(angle, refangle))/wavelength;
  rsFloat gain = Ge*std::pow(2*::j1c(x), 2);
  return gain*GetEfficiencyFactor();
}
