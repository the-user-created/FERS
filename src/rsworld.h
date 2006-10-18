//rsworld.h
//Simulator World Object
//The World contains all the other objects in the simulator's worldview
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 25 April 2006

#ifndef __RSWORLD_H
#define __RSWORLD_H

#include <config.h>
#include <vector>
#include <map>

namespace rs {

//Forward declaration of Receiver and Transmitter (rsradar.h)
class Receiver;
class Transmitter;
class PulseTransmitter;
class CWTransmitter;
//Forward declaration of RadarWaveform (rsradarwaveform.h)
class RadarWaveform;
class CWWaveform;
//Forward declaration of Antenna (rsantenna.h)
class Antenna;
//Forward declaration of Target (rstarget.h)
class Target;
//Forward declaration of Platform (rsplatform.h)
class Platform;
//Forward declaration of Timing (rstiming.h)
class Timing;

  /// World contains describes the parameters of the simulation, including objects.
  class World {
  public:
    /// Default Constructor
    World();
    /// Destructor
    ~World();
    /* Add a objects to the world.
	When Add is called with an object's pointer, the World takes
	control over that objects resources and will delete the object
	when necessary */
    /// Add a platform to the world    
    void Add(Platform *plat);
    /// Add a transmitter to the world
    void Add(Transmitter *trans);
    /// Add a reciver to the world
    void Add(Receiver *recv);
    /// Add a simple point target to the world
    void Add(Target *target);
    /// Add a pulse type to the world
    void Add(RadarWaveform *pulse);
    /// Add a CW waveform to the world
    void Add(CWWaveform *wave);
    /// Add an antenna to the world
    void Add(Antenna *antenna);
    /// Add a timing source to the world
    void Add(Timing *timing);

    /// Find a pulse with the specified name
    RadarWaveform* FindPulse(const std::string& name);
    /// Find a CW waveform with the specified name
    RadarWaveform* FindCWWaveform(const std::string& name);
    /// Find an antenna with the specified name
    Antenna* FindAntenna(const std::string& name);
    /// Find a timing source with the specified name
    Timing* FindTiming(const std::string& name);
    
    friend void RunThreadedSim(int thread_limit, World *world);
    friend void SimulatePairPulse(const PulseTransmitter *trans, Receiver *recv, const World *world);
    friend void SimulatePairCW(const CWTransmitter *trans, Receiver *recv, const World *world);

  protected:
    std::vector<Platform*> platforms; //!< Vector of all platforms in the world
    std::vector<Transmitter*> transmitters; //!< Vector of all transmitters in the world
    std::vector<Receiver*> receivers; //!< Vector of all receivers in the world
    std::vector<Target*> targets; //!< Vector of all targets in the world
    std::map<std::string, RadarWaveform*> pulses; //!< Vector of all pulses in the world
    std::map<std::string, CWWaveform*> cwwaves; //!< Map of CW waveforms to names    
    std::map<std::string, Antenna*> antennas; //!< Map of antennas to names
    std::map<std::string, Timing*> timings; //!< Map of timing sources to names
  };  

}


#endif
