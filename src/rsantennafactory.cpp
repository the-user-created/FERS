//rsantennafactory.cpp
//Functions for creating Antenna objects
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//24 July 2006

#include "rsantenna.h"
#include "rsantennafactory.h"

using namespace rs;

//Create an isotropic antenna with the specified name
Antenna* rs::CreateIsotropicAntenna(std::string name)
{
  rsAntenna::Isotropic *iso = new rsAntenna::Isotropic(name);
  return iso;
}

//Create a Sinc pattern antenna with the specified name, alpha and beta
Antenna* rs::CreateSincAntenna(std::string name, rsFloat alpha, rsFloat beta, rsFloat gamma)
{
  rsAntenna::Sinc *sinc = new rsAntenna::Sinc(name, alpha, beta, gamma);
  return sinc;
}

//Create a square horn antenna
Antenna* rs::CreateHornAntenna(std::string name, rsFloat dimension)
{
  rsAntenna::SquareHorn *sq = new rsAntenna::SquareHorn(name, dimension);
  return sq;
}

//Create a parabolic reflector antenna
Antenna* rs::CreateParabolicAntenna(std::string name, rsFloat diameter)
{
  rsAntenna::ParabolicReflector *pd = new rsAntenna::ParabolicReflector(name, diameter);
  return pd;
}
