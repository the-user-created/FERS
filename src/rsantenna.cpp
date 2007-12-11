//rsantenna.cpp
//Implementation of Antenna Class
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//20 July 2006

#define TIXML_USE_STL //Tell tinyxml to use the STL instead of it's own string class

#include <stdexcept>
#include "rsantenna.h"
#include "rsdebug.h"
#include <cmath>
#include "rsportable.h"
#include "rsinterp.h"
#include "tinyxml/tinyxml.h"

using namespace rs;
using namespace rsAntenna;

//One of the xml utility functions from xmlimport.cpp
rsFloat GetNodeFloat(TiXmlHandle &node);

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
Antenna::Antenna(const std::string& name):
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
Isotropic::Isotropic(const std::string& name):
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
Sinc::Sinc(const std::string& name, rsFloat alpha, rsFloat beta, rsFloat gamma):
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
SquareHorn::SquareHorn(const std::string& name, rsFloat dimension):
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
ParabolicReflector::ParabolicReflector(const std::string& name, rsFloat diameter):
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

//
// Antenna with pattern loaded from a file
//

// Constructor
FileAntenna::FileAntenna(const std::string& name, const std::string &filename):
  Antenna(name)
{
  // Classes to interpolate across elevation and azimuth
  azi_samples = new InterpSet();
  elev_samples = new InterpSet();
  //Load the XML antenna description data
  LoadAntennaDescription(filename);
}

//Destructor
FileAntenna::~FileAntenna()
{
  // Clean up the interpolation classes
  delete azi_samples;
  delete elev_samples;
}

//Return the gain of the antenna
//See doc/equations.tex for details
rsFloat FileAntenna::GetGain(const SVec3 &angle, const SVec3 &refangle, rsFloat wavelength) const
{
  SVec3 t_angle = angle-refangle;
  rsFloat azi_gain = azi_samples->Value(std::fabs(t_angle.azimuth));
  rsFloat elev_gain = elev_samples->Value(std::fabs(t_angle.elevation));
  return azi_gain*elev_gain*GetEfficiencyFactor();
}

namespace {

//Load samples of gain along an axis (not a member of FileAntenna)
void LoadAntennaGainAxis(InterpSet *set, TiXmlHandle &axisXML)
{
  rsFloat angle;
  rsFloat gain;
  //Step through the XML file and load all the gain samples
  TiXmlHandle tmp = axisXML.ChildElement("gainsample", 0);
  for (int i = 0; tmp.Element() != 0; i++) {
    //Load the angle of the gain sample
    TiXmlHandle angleXML = tmp.ChildElement("angle", 0);
    if (!angleXML.Element())
      throw std::runtime_error("[ERROR] Misformed XML in antenna description: No angle in gainsample");
    angle = GetNodeFloat(angleXML);
    //Load the gain of the gain sample
    TiXmlHandle gainXML = tmp.ChildElement("gain", 0);
    if (!gainXML.Element())
      throw std::runtime_error("[ERROR] Misformed XML in antenna description: No gain in gainsample");
    gain = GetNodeFloat(gainXML);
    //Load the values into the interpolation table
    set->InsertSample(angle, gain);
    //Get the next gainsample in the file
    tmp = axisXML.ChildElement("gainsample", i);
  }
}

}

//Load the antenna description file
void FileAntenna::LoadAntennaDescription(const std::string& filename)
{
  TiXmlDocument doc(filename.c_str());
  //Check the document was loaded correctly
  if (!doc.LoadFile())
    throw std::runtime_error("[ERROR] Could not load antenna description "+filename);
  //Get the XML root node
  TiXmlHandle root(doc.RootElement());
  //Load the gain samples along the elevation axis
  TiXmlHandle tmp = root.ChildElement("elevation", 0);
  if (!tmp.Element())
    throw std::runtime_error("[ERROR] Malformed XML in antenna description: No elevation pattern definition");
  LoadAntennaGainAxis(elev_samples, tmp);
  //Load the gain samples along the azimuth axis
  tmp = root.ChildElement("azimuth", 0);
  if (!tmp.Element())
    throw std::runtime_error("[ERROR] Malformed XML in antenna description: No azimuth pattern definition");
  LoadAntennaGainAxis(azi_samples, tmp);
}



//
// Antenna with gain pattern calculated by a Python program
//

// Constructor
PythonAntenna::PythonAntenna(const std::string& name, const std::string &module, const std::string& function):
  Antenna(name),
  py_antenna(module, function)
{
  
}

//Destructor
PythonAntenna::~PythonAntenna()
{
}

//Return the gain of the antenna
rsFloat PythonAntenna::GetGain(const SVec3 &angle, const SVec3 &refangle, rsFloat wavelength) const
{
  SVec3 angle_bore = angle - refangle; //Calculate the angle off boresight
  rsFloat gain = py_antenna.GetGain(angle_bore);
  return gain*GetEfficiencyFactor();
}


//
// Functions to create Antenna objects with a variety of properties
//

//Create an isotropic antenna with the specified name
Antenna* rs::CreateIsotropicAntenna(const std::string &name)
{
  rsAntenna::Isotropic *iso = new rsAntenna::Isotropic(name);
  return iso;
}

//Create a Sinc pattern antenna with the specified name, alpha and beta
Antenna* rs::CreateSincAntenna(const std::string &name, rsFloat alpha, rsFloat beta, rsFloat gamma)
{
  rsAntenna::Sinc *sinc = new rsAntenna::Sinc(name, alpha, beta, gamma);
  return sinc;
}

//Create a square horn antenna
Antenna* rs::CreateHornAntenna(const std::string &name, rsFloat dimension)
{
  rsAntenna::SquareHorn *sq = new rsAntenna::SquareHorn(name, dimension);
  return sq;
}

//Create a parabolic reflector antenna
Antenna* rs::CreateParabolicAntenna(const std::string &name, rsFloat diameter)
{
  rsAntenna::ParabolicReflector *pd = new rsAntenna::ParabolicReflector(name, diameter);
  return pd;
}

//Create an antenna with it's gain pattern stored in an XML file
Antenna* rs::CreateFileAntenna(const std::string &name, const std::string &file)
{
  rsAntenna::FileAntenna *fa = new rsAntenna::FileAntenna(name, file);
  return fa;
}

//Create an antenna with gain pattern described by a Python program
Antenna* rs::CreatePythonAntenna(const std::string &name, const std::string &module, const std::string &function)
{
  rsAntenna::PythonAntenna* pa = new rsAntenna::PythonAntenna(name, module, function);
  return pa;
}
