//rspulsefactory.h
//Functions for generating radar pulses from parameters
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//1 June 2006

//TODO: Extend this to support many pulse types

#ifndef __RS_PULSEFACTORY_H
#define __RS_PULSEFACTORY_H

#include <config.h>
#include <string>
#include "rsradarwaveform.h"


namespace rsPulseFactory {  

  /// Load a pulse from a file and generate an anypulse
  rs::RadarWaveform* GenerateAnyPulseFromFile(const std::string& name, const std::string& filename, rsFloat power, rsFloat carrierfreq);

  /// Generate a rect pulse of the specified length
  rs::RadarWaveform* GenerateRectAnyPulse(const std::string& name, rsFloat length, rsFloat power, rsFloat carrierfreq);

  /// Generate a rect pulse of the specified length
  rs::RadarWaveform* GenerateRectPulse(std::string name, rsFloat length, rsFloat power, rsFloat carrierfreq);

  /// Generate a monochrome CW
  rs::CWWaveform* GenerateMonoCW(std::string name, rsFloat power, rsFloat carrierfreq);

}

#endif
