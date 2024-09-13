// sim_threading.h
// Definitions for threaded simulation code
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 19 July 2006

#ifndef SIM_THREADING_H
#define SIM_THREADING_H

#include "radar/radar_system.h"

namespace rs
{
	class World;
	class Target;
}

namespace rs::threaded_sim
{
	struct ReResults
	{
		RS_FLOAT power, delay, doppler, phase, noise_temperature;
	};

	class RangeError final : public std::exception
	{
	public:
		[[nodiscard]] const char* what() const noexcept override
		{
			return "Range error in RE calculations";
		}
	};

	class Thread
	{
	public:
		static void decThreads();

		static void setError();
	};

	class SimThread : public Thread
	{
	public:
		SimThread(const Transmitter* transmitter, Receiver* receiver, const World* world)
			: _trans(transmitter), _recv(receiver), _world(world)
		{
		}

		void operator()() const;

	private:
		const Transmitter* _trans;
		Receiver* _recv;
		const World* _world;
	};

	class RenderThread : public Thread
	{
	public:
		explicit RenderThread(Receiver* recv) : _recv(recv)
		{
		}

		void operator()() const;

	private:
		Receiver* _recv;
	};

	void incThreads();

	void runThreadedSim(unsigned threadLimit, const World* world);

	void simulatePair(const Transmitter* trans, Receiver* recv, const World* world);

	void simulateTarget(const Transmitter* trans, Receiver* recv, const Target* targ, const TransmitterPulse* signal);

	void addDirect(const Transmitter* trans, Receiver* recv, const TransmitterPulse* signal);

	void solveRe(const Transmitter* trans, const Receiver* recv, const Target* targ, RS_FLOAT time, RS_FLOAT length,
	             const RadarSignal* wave, ReResults& results);

	void solveReDirect(const Transmitter* trans, const Receiver* recv, RS_FLOAT time, RS_FLOAT length,
	                   const RadarSignal* wave, ReResults& results);
}

#endif
