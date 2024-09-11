//rsworld.h
//Simulator World Object
//The World contains all the other objects in the simulator's worldview
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//Started: 25 April 2006

#ifndef RS_WORLD_H
#define RS_WORLD_H

#include <map>
#include <string>
#include <vector>

namespace rs
{
	//Forward declaration of Receiver and Transmitter (rsradar.h)
	class Receiver;
	class Transmitter;

	//Forward declaration of RadarSignal (rsradarwaveform.h)
	class RadarSignal;

	//Forward declaration of Antenna (rsantenna.h)
	class Antenna;
	//Forward declaration of Target (rstarget.h)
	class Target;
	//Forward declaration of Platform (rsplatform.h)
	class Platform;
	//Forward declaration of PrototypeTiming (rstiming.h)
	class PrototypeTiming;
	//Forward declaration of MultipathSurface (rsmultipath.h)
	class MultipathSurface;

	/// World contains describes the parameters of the simulation, including objects.
	class World
	{
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
		void add(Platform* plat);

		/// Add a transmitter to the world
		void add(Transmitter* trans);

		/// Add a reciver to the world
		void add(Receiver* recv);

		/// Add a simple point target to the world
		void add(Target* target);

		/// Add a pulse type to the world
		void add(RadarSignal* pulse);

		/// Add an antenna to the world
		void add(Antenna* antenna);

		/// Add a timing source to the world
		void add(PrototypeTiming* timing);

		/// Find a pulse with the specified name
		RadarSignal* findSignal(const std::string& name);

		/// Find an antenna with the specified name
		Antenna* findAntenna(const std::string& name);

		/// Find a timing source with the specified name
		PrototypeTiming* findTiming(const std::string& name);

		///Add a multipath surface to the world
		void addMultipathSurface(MultipathSurface* surface);

		///Process the scene to add virtual receivers and transmitters
		void processMultipath();

		friend void runThreadedSim(unsigned int threadLimit, World* world);

		friend void simulatePair(const Transmitter* trans, Receiver* recv, const World* world);

	protected:
		std::vector<Platform*> _platforms; //!< Vector of all platforms in the world
		std::vector<Transmitter*> _transmitters; //!< Vector of all transmitters in the world
		std::vector<Receiver*> _receivers; //!< Vector of all receivers in the world
		std::vector<Target*> _targets; //!< Vector of all targets in the world
		std::map<std::string, RadarSignal*> _pulses; //!< Vector of all signals in the world
		std::map<std::string, Antenna*> _antennas; //!< Map of antennas to names
		std::map<std::string, PrototypeTiming*> _timings; //!< Map of timing sources to names
		//We only support a single multipath surface
		MultipathSurface* _multipath_surface; //!< Surface to use for multipath propagation
	};
}

namespace rs
{
	void runThreadedSim(unsigned int threadLimit, World* world);
}

#endif
