// sim_threading.h
// Definitions for threaded simulation code
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 19 July 2006

#ifndef SIM_THREADING_H
#define SIM_THREADING_H

#include <exception>

#include "config.h"

namespace radar
{
	class Receiver;
	class Transmitter;
}

namespace core
{
	class World;

	struct ReResults
	{
		RS_FLOAT power, delay, doppler, phase, noise_temperature;
	};

	class RangeError final : public std::exception
	{
	public:
		[[nodiscard]] const char* what() const noexcept override { return "Range error in RE calculations"; }
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
		SimThread(const radar::Transmitter* transmitter, radar::Receiver* receiver, const World* world)
			: _trans(transmitter), _recv(receiver), _world(world) {}

		void operator()() const;

	private:
		const radar::Transmitter* _trans;
		radar::Receiver* _recv;
		const World* _world;
	};

	class RenderThread : public Thread
	{
	public:
		explicit RenderThread(radar::Receiver* recv) : _recv(recv) {}

		void operator()() const;

	private:
		radar::Receiver* _recv;
	};

	void runThreadedSim(unsigned threadLimit, const World* world);
}

#endif
