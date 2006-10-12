//rsantennafactory.cpp
//Functions which create Antenna objects
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//24 July 2006

#ifndef __RSANTENNAFACTORY_H
#define __RSANTENNAFACTORY_H

#include <config.h>
#include <string>

namespace rs {

class Antenna; //See rsantenna.h

/// Create an Isotropic Antenna
Antenna* CreateIsotropicAntenna(std::string name);

/// Create a Sinc Pattern Antenna
// see rsantenna.cpp for meaning of alpha and beta
Antenna* CreateSincAntenna(std::string name, rsFloat alpha, rsFloat beta, rsFloat gamma);

/// Create a Square Horn Antenna
Antenna* CreateHornAntenna(std::string name, rsFloat dimension);

/// Create a parabolic reflector dish
Antenna* CreateParabolicAntenna(std::string name, rsFloat diameter);

}

#endif
