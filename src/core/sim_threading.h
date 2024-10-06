// sim_threading.h
// Definitions for threaded simulation code
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 19 July 2006

#pragma once

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
		RealType power, delay, doppler, phase, noise_temperature;
	};

	class RangeError final : public std::exception
	{
	public:
		[[nodiscard]] const char* what() const noexcept override { return "Range error in RE calculations"; }
	};

	class SimThread
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

	class RenderThread
	{
	public:
		explicit RenderThread(radar::Receiver* recv) : _recv(recv) {}

		void operator()() const;

	private:
		radar::Receiver* _recv;
	};

	void runThreadedSim(unsigned threadLimit, const World* world);
}
