// sim_threading.cpp
// Thread management for the simulator
// Marc Brooker mbrooker@rrsg.ee.uct.ac.za
// 29 May 2006

// One of the goals for FERS is to support multiple processors.
// This is achieved through multithreading.
// One simulation is performed in for each transmitter-receiver pair.
// Multi-threading runs a number of these simulations in parallel,
// according to the number of CPUs (or cores), the system has.

#include "sim_threading.h"

#include <boost/thread/thread.hpp>

#include "core/logging.h"
#include "core/parameters.h"
#include "radar/target.h"
#include "core/world.h"

// TODO: Is this BOOST_VERSION check necessary?
#if BOOST_VERSION < 105000
#define TIME_UTC_ TIME_UTC
#endif

// Counter of currently running threads
volatile int threads;
boost::mutex threads_mutex;

// Flag to set if a thread encounters an error
volatile int error;
boost::mutex error_mutex;

namespace rs::threaded_sim
{
	void SimThread::operator()() const
	{
		logging::printf(logging::RS_VERBOSE,
		                "[VERBOSE] Created simulator thread for transmitter '%s' and receiver '%s'\n",
		                _trans->getName().c_str(), _recv->getName().c_str());
		try
		{
			simulatePair(_trans, _recv, _world);
		}
		catch (std::exception& ex)
		{
			logging::printf(logging::RS_CRITICAL,
			                "[ERROR] First pass thread terminated with unexpected error:\n\t%s\nSimulator will terminate\n",
			                ex.what());
			setError();
		}
		decThreads();
	}

	void RenderThread::operator()() const
	{
		logging::printf(logging::RS_VERY_VERBOSE, "[VV] Created render thread for receiver '%s'\n",
		                _recv->getName().c_str());
		try
		{
			_recv->render();
		}
		catch (std::exception& ex)
		{
			logging::printf(logging::RS_CRITICAL,
			                "[ERROR] Render thread terminated with unexpected error:\n\t%s\nSimulator will terminate\n",
			                ex.what());
			setError();
		}
		decThreads();
	}

	// Run a sim thread for each of the receiver-transmitter pairs, limiting concurrent threads
	void runThreadedSim(const unsigned threadLimit, const World* world)
	{
		std::vector<std::unique_ptr<boost::thread>> running;
		logging::printf(logging::RS_INFORMATIVE, "[INFO] Using threaded simulation with %d threads.\n", threadLimit);
		std::vector<Receiver*> receivers = world->getReceivers();

		for (const auto& receiver : receivers)
		{
			std::vector<Transmitter*> transmitters = world->getTransmitters();
			for (const auto& transmitter : transmitters)
			{
				incThreads();
				SimThread sim(transmitter, receiver, world);
				auto thrd = std::make_unique<boost::thread>(sim);
				while (threads >= threadLimit)
				{
					boost::thread::yield();
				}
				if (error)
				{
					throw std::runtime_error("Thread terminated with error. Aborting simulation");
				}
				running.push_back(std::move(thrd));
			}
		}
		while (threads)
		{
			boost::thread::yield();
			if (error)
			{
				throw std::runtime_error("Thread terminated with error. Aborting simulation");
			}
		}
		running.clear();

		for (const auto& receiver : receivers)
		{
			logging::printf(logging::RS_VERY_VERBOSE, "[VV] %d responses added to receiver '%s'\n",
			                receiver->countResponses(), receiver->getName().c_str());
		}

		for (const auto& receiver : receivers)
		{
			incThreads();
			RenderThread sim(receiver);
			auto thrd = std::make_unique<boost::thread>(sim);
			while (threads >= threadLimit)
			{
				boost::thread::yield();
			}
			if (error)
			{
				throw std::runtime_error("Thread terminated with error. Aborting simulation");
			}
			running.push_back(std::move(thrd));
		}

		while (threads)
		{
			boost::thread::yield();
			if (error)
			{
				throw std::runtime_error("Thread terminated with error. Aborting simulation");
			}
		}
		running.clear();
	}

	void simulatePair(const Transmitter* trans, Receiver* recv, const World* world)
	{
		const int pulses = trans->getPulseCount();
		auto* pulse = new TransmitterPulse();
		for (int i = 0; i < pulses; i++)
		{
			trans->getPulse(pulse, i);
			for (const auto target : world->getTargets())
			{
				simulateTarget(trans, recv, target, pulse);
			}
			if (!recv->checkFlag(Receiver::FLAG_NODIRECT))
			{
				addDirect(trans, recv, pulse);
			}
		}
		delete pulse;
	}

	void addDirect(const Transmitter* trans, Receiver* recv, const TransmitterPulse* signal)
	{
		if (trans->isMonostatic() && trans->getAttached() == recv)
		{
			return;
		}
		const RS_FLOAT start_time = signal->time;
		const RS_FLOAT end_time = signal->time + signal->wave->getLength();
		const RS_FLOAT sample_time = 1.0 / parameters::cwSampleRate();
		const RS_FLOAT point_count = std::ceil(signal->wave->getLength() / sample_time);
		auto* response = new Response(signal->wave, trans);
		try
		{
			for (int i = 0; i < point_count; i++)
			{
				const RS_FLOAT stime = i * sample_time + start_time;
				ReResults results{};
				solveReDirect(trans, recv, stime, sample_time, signal->wave, results);
				InterpPoint point(results.power, results.delay + stime, results.delay, results.doppler, results.phase,
				                  results.noise_temperature);
				response->addInterpPoint(point);
			}
			ReResults results{};
			solveReDirect(trans, recv, end_time, sample_time, signal->wave, results);
			const InterpPoint point(results.power, results.delay + end_time, results.delay, results.doppler,
			                        results.phase, results.noise_temperature);
			response->addInterpPoint(point);
		}
		catch (RangeError&)
		{
			throw std::runtime_error("Receiver or Transmitter too close to Target for accurate simulation");
		}
		recv->addResponse(response);
	}

	void simulateTarget(const Transmitter* trans, Receiver* recv, const Target* targ, const TransmitterPulse* signal)
	{
		const RS_FLOAT start_time = signal->time;
		const RS_FLOAT end_time = signal->time + signal->wave->getLength();
		const RS_FLOAT sample_time = 1.0 / parameters::cwSampleRate();
		const RS_FLOAT point_count = std::ceil(signal->wave->getLength() / sample_time);
		auto* response = new Response(signal->wave, trans);
		try
		{
			for (int i = 0; i < point_count; i++)
			{
				const RS_FLOAT stime = i * sample_time + start_time;
				ReResults results{};
				solveRe(trans, recv, targ, stime, sample_time, signal->wave, results);
				InterpPoint point(results.power, stime + results.delay, results.delay, results.doppler, results.phase,
				                  results.noise_temperature);
				response->addInterpPoint(point);
			}
			ReResults results{};
			solveRe(trans, recv, targ, end_time, sample_time, signal->wave, results);
			const InterpPoint point(results.power, end_time + results.delay, results.delay, results.doppler,
			                        results.phase, results.noise_temperature);
			response->addInterpPoint(point);
		}
		catch (RangeError&)
		{
			throw std::runtime_error("Receiver or Transmitter too close to Target for accurate simulation");
		}
		recv->addResponse(response);
	}

	void solveRe(const Transmitter* trans, const Receiver* recv, const Target* targ, const RS_FLOAT time,
	             const RS_FLOAT length, const RadarSignal* wave, ReResults& results)
	{
		const Vec3 transmitter_position = trans->getPosition(time);
		const Vec3 receiver_position = recv->getPosition(time);
		const Vec3 target_position = targ->getPosition(time);
		auto transmitter_to_target_vector = SVec3(target_position - transmitter_position);
		auto receiver_to_target_vector = SVec3(target_position - receiver_position);
		const RS_FLOAT transmitter_to_target_distance = transmitter_to_target_vector.length;
		const RS_FLOAT receiver_to_target_distance = receiver_to_target_vector.length;
		transmitter_to_target_vector.length = 1;
		receiver_to_target_vector.length = 1;
		if (transmitter_to_target_distance <= std::numeric_limits<RS_FLOAT>::epsilon() || receiver_to_target_distance <=
			std::numeric_limits<RS_FLOAT>::epsilon())
		{
			throw RangeError();
		}
		results.delay = (transmitter_to_target_distance + receiver_to_target_distance) / parameters::c();
		const RS_FLOAT rcs = targ->getRcs(transmitter_to_target_vector, receiver_to_target_vector);
		const RS_FLOAT wavelength = parameters::c() / wave->getCarrier();
		const RS_FLOAT transmitter_gain = trans->getGain(transmitter_to_target_vector, trans->getRotation(time),
		                                                 wavelength);
		const RS_FLOAT receiver_gain = recv->getGain(receiver_to_target_vector, recv->getRotation(results.delay + time),
		                                             wavelength);
		results.power = transmitter_gain * receiver_gain * rcs / (4 * M_PI);
		if (!recv->checkFlag(Receiver::FLAG_NOPROPLOSS))
		{
			results.power *= wavelength * wavelength / (pow(4 * M_PI, 2) * transmitter_to_target_distance *
				transmitter_to_target_distance * receiver_to_target_distance * receiver_to_target_distance);
		}
		if (trans->isMultipathDual())
		{
			results.power *= trans->multipathDualFactor();
		}
		if (recv->isMultipathDual())
		{
			results.power *= trans->multipathDualFactor();
		}
		results.phase = -results.delay * 2 * M_PI * wave->getCarrier();
		const Vec3 trpos_end = trans->getPosition(time + length);
		const Vec3 repos_end = recv->getPosition(time + length);
		const Vec3 tapos_end = targ->getPosition(time + length);
		const auto transvec_end = SVec3(tapos_end - trpos_end);
		const auto recvvec_end = SVec3(tapos_end - repos_end);
		const RS_FLOAT rt_end = transvec_end.length;
		const RS_FLOAT rr_end = recvvec_end.length;
		if (rt_end < std::numeric_limits<RS_FLOAT>::epsilon() || rr_end < std::numeric_limits<RS_FLOAT>::epsilon())
		{
			throw std::runtime_error("Target is too close to transmitter or receiver for accurate simulation");
		}
		const RS_FLOAT v_r = (rr_end - receiver_to_target_distance) / length;
		const RS_FLOAT v_t = (rt_end - transmitter_to_target_distance) / length;
		results.doppler = std::sqrt((1 + v_r / parameters::c()) / (1 - v_r / parameters::c())) * std::sqrt(
			(1 + v_t / parameters::c()) / (1 - v_t / parameters::c()));
		results.noise_temperature = recv->getNoiseTemperature(recv->getRotation(time + results.delay));
	}

	void solveReDirect(const Transmitter* trans, const Receiver* recv, const RS_FLOAT time, const RS_FLOAT length,
	                   const RadarSignal* wave, ReResults& results)
	{
		const Vec3 tpos = trans->getPosition(time);
		const Vec3 rpos = recv->getPosition(time);
		auto transvec = SVec3(tpos - rpos);
		auto recvvec = SVec3(rpos - tpos);
		const RS_FLOAT r = transvec.length;
		transvec.length = 1;
		recvvec.length = 1;
		if (r > std::numeric_limits<RS_FLOAT>::epsilon())
		{
			results.delay = r / parameters::c();
			const RS_FLOAT wl = parameters::c() / wave->getCarrier();
			const RS_FLOAT gt = trans->getGain(transvec, trans->getRotation(time), wl);
			const RS_FLOAT gr = recv->getGain(recvvec, recv->getRotation(time + results.delay), wl);
			results.power = gt * gr * wl * wl / (4 * M_PI);
			if (!recv->checkFlag(Receiver::FLAG_NOPROPLOSS))
			{
				results.power *= 1 / (4 * M_PI * pow(r, 2));
			}
			const Vec3 tpos_end = trans->getPosition(time + length);
			const Vec3 rpos_end = recv->getPosition(time + length);
			const Vec3 trpos_end = tpos_end - rpos_end;
			const RS_FLOAT r_end = trpos_end.length();
			const RS_FLOAT vdoppler = (r_end - r) / length;
			results.doppler = (parameters::c() + vdoppler) / (parameters::c() - vdoppler);
			if (trans->isMultipathDual())
			{
				results.power = 0;
			}
			if (recv->isMultipathDual())
			{
				results.power = 0;
			}
			results.phase = fmod(results.delay * 2 * M_PI * wave->getCarrier(), 2 * M_PI);
			results.noise_temperature = recv->getNoiseTemperature(recv->getRotation(time + results.delay));
		}
		else
		{
			throw RangeError();
		}
	}

	void incThreads()
	{
		boost::mutex::scoped_lock lock(threads_mutex);
		threads++;
	}

	void Thread::decThreads()
	{
		boost::mutex::scoped_lock lock(threads_mutex);
		threads--;
	}

	void Thread::setError()
	{
		boost::mutex::scoped_lock lock(error_mutex);
		error = 1;
	}
}
