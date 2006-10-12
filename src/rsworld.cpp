//rsworld.cpp
//Implementation of simulator world object
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 25 April 2006
#include <algorithm>
#include "rsdebug.h"
#include "rsworld.h"
#include "rsantenna.h"
#include "rsradar.h"
#include "rsantenna.h"
#include "rsradarwaveform.h"
#include "rsplatform.h"
#include "rstarget.h"

using namespace rs; //Import the rs namespace for this implementation

/// Function object used to delete objects from vector
  template <typename T> struct objDel
  {
    //operator() deletes the object that x is a pointer to
    void operator()(T x) {
      delete x;
    }
  };


//Default constructor
World::World()
{
  antennas.clear();
  cwwaves.clear();
  pulses.clear();
}

//World object destructor
World::~World()
{
  //Delete all the objects in the world 
  std::map<std::string, RadarWaveform*>::iterator iter;
  for(iter = pulses.begin(); iter != pulses.end(); iter++)
    delete (*iter).second;
  pulses.clear();

  std::map<std::string, CWWaveform*>::iterator witer;
  for(witer = cwwaves.begin(); witer != cwwaves.end(); witer++)
    delete (*witer).second;
  cwwaves.clear();

  std::map<std::string, Antenna*>::iterator aiter;
  for (aiter = antennas.begin(); aiter != antennas.end(); aiter++)
    delete (*aiter).second;
  antennas.clear();

  std::for_each(receivers.begin(), receivers.end(), objDel<Receiver *>());
  receivers.clear();
  std::for_each(transmitters.begin(), transmitters.end(), objDel<Transmitter *>());
  transmitters.clear();
  std::for_each(targets.begin(), targets.end(), objDel<Target *>());
  targets.clear();
  //Platforms are deleted last as they are referred to by the other object types
  std::for_each(platforms.begin(), platforms.end(), objDel<Platform *>());
  platforms.clear();

}

//Add a platform to the world
void World::Add(Platform *platform)
{
  platforms.push_back(platform);
}

//Add a transmitter to the world
void World::Add(Transmitter *trans)
{
  transmitters.push_back(trans);
}

//Add a receiver to the world
void World::Add(Receiver *recv)
{
  receivers.push_back(recv);
}

//Add a target to the world
void World::Add(Target *targ)
{
  targets.push_back(targ);
}

//Add a pulse to the world
void World::Add(RadarWaveform *pulse)
{
  pulses[pulse->GetName()] = pulse;
}

//Add a CW waveform to the world
void World::Add(CWWaveform *wave)
{
  cwwaves[wave->GetName()] = wave;
}

//Add an antenna to the world
void World::Add(Antenna *antenna)
{
  antennas[antenna->GetName()] = antenna;
}

//Get a pulse from the map of pulses
RadarWaveform* World::FindPulse(std::string name)
{
  return pulses[name];
}

//Get a cw waveform from the map
RadarWaveform* World::FindCWWaveform(std::string name)
{
  return cwwaves[name];
}

//Get an antenna from the map of antennas
Antenna* World::FindAntenna(std::string name)
{
    return antennas[name];
}
