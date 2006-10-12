//rstarget.cpp
//Implements the target class
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za

#include "rstarget.h"
#include "rsdebug.h"

using namespace rs;

//Default constructor for Target object
Target::Target(Platform *platform, std::string name):
  Object(platform, name),
  pattern(Target::RCS_ISOTROPIC),
  rcs(0)
{
}

//Default destructor for Target object
Target::~Target()
{
}

//Get the radar cross section at a particular angle
rsFloat Target::GetRCS(SVec3 &inAngle, SVec3 &outAngle) const
{
  switch (pattern) {
  case RCS_ISOTROPIC:
    return rcs; //Isotropic RCS is the same at all angles
  case RCS_COSINE:
    return CalcCosRCS(inAngle, outAngle); //Calculate cosine RCS pattern
  case RCS_ARBITRARY:
    return CalcPatternRCS(inAngle, outAngle); //Calculate RCS based on a pattern
  }
  DEBUG_PRINT(rsDebug::RS_IMPORTANT, "[BUG] Attempted to use unsupported gain pattern. Simulation results are likely to be incorrect");
  return 0;
}

//Set the radar cross section of the target
void Target::SetRCS(rsFloat newRCS, RCSPattern newPattern)
{
  //Set the pattern
  pattern = newPattern;

  //Set the rcs value
  if (newRCS <= 0)
    DEBUG_PRINT(rsDebug::RS_INFORMATIVE, "Refusing to set negative or zero Radar Cross Section");  else
    rcs = newRCS;
}

//Calculate RCS based on the cos RCS pattern
rsFloat Target::CalcCosRCS(SVec3 &inAngle, SVec3 &outAngle) const
{
  DEBUG_PRINT(rsDebug::RS_CRITICAL, "[TODO] Cosine RCS pattern not implemented. Results of simulation are likely to be incorrect");
  return 0;
}

//Calculate the RCS based on an arbitrary gain pattern
rsFloat Target::CalcPatternRCS(SVec3 &inAngle, SVec3 &outAngle) const
{
  DEBUG_PRINT(rsDebug::RS_CRITICAL, "[TODO] RCS Pattern not implemented. Results of simulation are likely to be incorrect");
  return 0;
}
