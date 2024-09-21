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

#include <functional>
#include <thread>

#include "parameters.h"
#include "world.h"
#include "math_utils/geometry_ops.h"
#include "radar/radar_system.h"
#include "radar/target.h"
#include "signal_processing/radar_signal.h"

using radar::Transmitter;
using radar::Receiver;
using radar::Target;
using math::SVec3;
using signal::RadarSignal;
using radar::TransmitterPulse;
using serial::Response;
using logging::Level;

// Counter of currently running threads
std::atomic threads{0};
std::mutex threads_mutex;

// Flag to set if a thread encounters an error
std::atomic error{0};
std::mutex error_mutex;

namespace
{
	// =================================================================================================================
	//
	// THREAD CLASSES HELPERS
	//
	// =================================================================================================================

	void solveRe(const Transmitter* trans, const Receiver* recv, const Target* targ, const RS_FLOAT time,
	             const RS_FLOAT length, const RadarSignal* wave, core::ReResults& results)
	{
		// Get initial positions and distance vectors
		const auto transmitter_position = trans->getPosition(time);
		const auto receiver_position = recv->getPosition(time);
		const auto target_position = targ->getPosition(time);

		auto transmitter_to_target_vector = SVec3(target_position - transmitter_position);
		auto receiver_to_target_vector = SVec3(target_position - receiver_position);

		const auto transmitter_to_target_distance = transmitter_to_target_vector.length;
		const auto receiver_to_target_distance = receiver_to_target_vector.length;

		if (transmitter_to_target_distance <= std::numeric_limits<RS_FLOAT>::epsilon() ||
			receiver_to_target_distance <= std::numeric_limits<RS_FLOAT>::epsilon()) { throw core::RangeError(); }

		// Normalize distance vectors
		transmitter_to_target_vector.length = 1;
		receiver_to_target_vector.length = 1;

		results.delay = (transmitter_to_target_distance + receiver_to_target_distance) / params::c();

		const auto rcs = targ->getRcs(transmitter_to_target_vector, receiver_to_target_vector);
		const auto wavelength = params::c() / wave->getCarrier();

		const auto transmitter_gain = trans->
			getGain(transmitter_to_target_vector, trans->getRotation(time), wavelength);
		const auto receiver_gain = recv->getGain(receiver_to_target_vector, recv->getRotation(results.delay + time),
		                                         wavelength);

		results.power = transmitter_gain * receiver_gain * rcs / (4 * M_PI);

		// Propagation loss calculation
		if (!recv->checkFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS))
		{
			const RS_FLOAT distance_product = transmitter_to_target_distance * receiver_to_target_distance;
			results.power *= wavelength * wavelength / (pow(4 * M_PI, 2) * distance_product * distance_product);
		}

		// Apply multipath factors if applicable
		if (trans->getMultipathDual()) { results.power *= trans->getMultipathFactor(); }
		if (recv->getMultipathDual()) { results.power *= recv->getMultipathFactor(); }

		results.phase = -results.delay * 2 * M_PI * wave->getCarrier();

		// End position and Doppler calculation
		const auto transvec_end = SVec3(targ->getPosition(time + length) - trans->getPosition(time + length));
		const auto recvvec_end = SVec3(targ->getPosition(time + length) - recv->getPosition(time + length));

		const RS_FLOAT rt_end = transvec_end.length;
		const RS_FLOAT rr_end = recvvec_end.length;

		if (rt_end < std::numeric_limits<RS_FLOAT>::epsilon() || rr_end < std::numeric_limits<RS_FLOAT>::epsilon())
		{
			throw std::runtime_error("Target is too close to transmitter or receiver for accurate simulation");
		}

		const RS_FLOAT v_r = (rr_end - receiver_to_target_distance) / length;
		const RS_FLOAT v_t = (rt_end - transmitter_to_target_distance) / length;

		results.doppler = std::sqrt((1 + v_r / params::c()) / (1 - v_r / params::c())) *
			std::sqrt((1 + v_t / params::c()) / (1 - v_t / params::c()));

		// Set noise temperature
		results.noise_temperature = recv->getNoiseTemperature(recv->getRotation(time + results.delay));
	}

	void solveReDirect(const Transmitter* trans, const Receiver* recv, const RS_FLOAT time,
	                   const RS_FLOAT length, const RadarSignal* wave, core::ReResults& results)
	{
		const auto tpos = trans->getPosition(time);
		const auto rpos = recv->getPosition(time);

		const auto transvec = SVec3(tpos - rpos);
		const RS_FLOAT distance = transvec.length;

		if (distance <= std::numeric_limits<RS_FLOAT>::epsilon()) { throw core::RangeError(); }

		results.delay = distance / params::c();

		const RS_FLOAT wavelength = params::c() / wave->getCarrier();
		const RS_FLOAT transmitter_gain = trans->getGain(transvec, trans->getRotation(time), wavelength);
		const RS_FLOAT receiver_gain = recv->getGain(SVec3(rpos - tpos), recv->getRotation(time + results.delay),
		                                             wavelength);

		results.power = transmitter_gain * receiver_gain * wavelength * wavelength / (4 * M_PI);

		if (!recv->checkFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS)) { results.power /= 4 * M_PI * distance * distance; }

		// Doppler shift calculation
		const auto trpos_end = trans->getPosition(time + length) - recv->getPosition(time + length);
		const RS_FLOAT r_end = trpos_end.length();
		const RS_FLOAT doppler_shift = (r_end - distance) / length;
		results.doppler = (params::c() + doppler_shift) / (params::c() - doppler_shift);

		// Multipath conditions
		if (trans->getMultipathDual() || recv->getMultipathDual()) { results.power = 0; }

		results.phase = fmod(results.delay * 2 * M_PI * wave->getCarrier(), 2 * M_PI);
		results.noise_temperature = recv->getNoiseTemperature(recv->getRotation(time + results.delay));
	}

	void addDirect(const Transmitter* trans, Receiver* recv, const TransmitterPulse* signal)
	{
		// Exit early if monostatic and the transmitter is attached to the receiver
		if (trans->isMonostatic() && trans->getAttached() == recv) { return; }

		const RS_FLOAT start_time = signal->time;
		const RS_FLOAT end_time = start_time + signal->wave->getLength();
		const RS_FLOAT sample_time = 1.0 / params::cwSampleRate();
		const int point_count = static_cast<int>(std::ceil(signal->wave->getLength() / sample_time));

		// Create a response object for this signal
		auto response = std::make_unique<Response>(signal->wave, trans);

		try
		{
			// Iterate through sample points
			for (int i = 0; i <= point_count; ++i)
			{
				const RS_FLOAT current_time = i < point_count ? start_time + i * sample_time : end_time;

				// Compute simulation results
				core::ReResults results{};
				solveReDirect(trans, recv, current_time, sample_time, signal->wave, results);

				// Add interpolation point
				interp::InterpPoint point(results.power, results.delay + current_time, results.delay, results.doppler,
				                          results.phase, results.noise_temperature);
				response->addInterpPoint(point);
			}
		}
		catch (core::RangeError&)
		{
			throw std::runtime_error("Receiver or Transmitter too close to Target for accurate simulation");
		}

		// Add response to receiver
		recv->addResponse(std::move(response));
	}

	void simulateTarget(const Transmitter* trans, Receiver* recv, const Target* targ,
	                    const TransmitterPulse* signal)
	{
		const RS_FLOAT start_time = signal->time;
		const RS_FLOAT end_time = start_time + signal->wave->getLength();
		const RS_FLOAT sample_time = 1.0 / params::cwSampleRate();
		const int point_count = static_cast<int>(std::ceil(signal->wave->getLength() / sample_time));

		auto response = std::make_unique<Response>(signal->wave, trans);

		try
		{
			// Loop over all time points, including the final one
			for (int i = 0; i <= point_count; ++i)
			{
				const RS_FLOAT current_time = i < point_count ? start_time + i * sample_time : end_time;

				// Simulate the results for each time step
				core::ReResults results{};
				solveRe(trans, recv, targ, current_time, sample_time, signal->wave, results);

				// Add interpolation point to the response
				interp::InterpPoint point(results.power, current_time + results.delay, results.delay, results.doppler,
				                          results.phase, results.noise_temperature);
				response->addInterpPoint(point);
			}
		}
		catch (core::RangeError&)
		{
			throw std::runtime_error("Receiver or Transmitter too close to Target for accurate simulation");
		}

		// Add the response to the receiver
		recv->addResponse(std::move(response));
	}

	void simulatePair(const Transmitter* trans, Receiver* recv, const core::World* world)
	{
		const int pulses = trans->getPulseCount();
		const auto pulse = std::make_unique<TransmitterPulse>();
		for (int i = 0; i < pulses; i++)
		{
			trans->getPulse(pulse.get(), i);
			for (const auto& target : world->getTargets()) { simulateTarget(trans, recv, target.get(), pulse.get()); }
			if (!recv->checkFlag(Receiver::RecvFlag::FLAG_NODIRECT)) { addDirect(trans, recv, pulse.get()); }
		}
	}

	// =================================================================================================================
	//
	// RUN THREADED SIMULATION HELPERS
	//
	// =================================================================================================================

	// Helper function to check for errors in threads
	void checkForError()
	{
		if (error) { throw std::runtime_error("Thread terminated with error. Aborting simulation"); }
	}

	void incThreads()
	{
		std::lock_guard lock(threads_mutex);
		++threads;
	}

	// Helper function to start a simulation thread
	void startSimThread(const unsigned threadLimit, std::vector<std::unique_ptr<std::thread>>& running,
	                    const std::function<std::unique_ptr<std::thread>()>& createThread)
	{
		incThreads();
		auto thrd = createThread();
		while (static_cast<unsigned>(threads) >= threadLimit) { std::this_thread::yield(); }
		checkForError();
		running.push_back(std::move(thrd));
	}

	// Helper function to wait for threads to finish
	void waitForThreadsToFinish(std::vector<std::unique_ptr<std::thread>>& running)
	{
		for (const auto& thrd : running) { if (thrd->joinable()) { thrd->join(); } }
		running.clear();
	}

	// Helper function to run simulation for receiver-transmitter pairs
	void runSimForReceiverTransmitterPairs(const unsigned threadLimit, const core::World* world,
	                                       const std::vector<std::unique_ptr<Receiver>>& receivers,
	                                       std::vector<std::unique_ptr<std::thread>>& running)
	{
		const auto& transmitters = world->getTransmitters();
		for (const auto& receiver : receivers)
		{
			for (const auto& transmitter : transmitters)
			{
				startSimThread(threadLimit, running, [&]
				{
					return std::make_unique<std::thread>(
						core::SimThread(transmitter.get(), receiver.get(), world));
				});
			}
		}
		waitForThreadsToFinish(running);
	}

	// Helper function to finalize receiver responses and log
	void finalizeReceiverResponses(const std::vector<std::unique_ptr<Receiver>>& receivers)
	{
		for (const auto& receiver : receivers)
		{
			LOG(Level::DEBUG, "{} responses added to receiver '{}'",
			    receiver->countResponses(), receiver->getName().c_str());
		}
	}

	// Helper function to run rendering threads for receivers
	void runRenderThreads(const unsigned threadLimit, const std::vector<std::unique_ptr<Receiver>>& receivers,
	                      std::vector<std::unique_ptr<std::thread>>& running)
	{
		for (const auto& receiver : receivers)
		{
			startSimThread(threadLimit, running, [&]
			{
				return std::make_unique<std::thread>(core::RenderThread(receiver.get()));
			});
		}
		waitForThreadsToFinish(running);
	}
}

namespace core
{
	// =================================================================================================================
	//
	// THREAD
	//
	// =================================================================================================================

	void Thread::decThreads()
	{
		std::lock_guard lock(threads_mutex);
		--threads;
	}

	void Thread::setError()
	{
		std::lock_guard lock(error_mutex);
		error = 1;
	}

	// =================================================================================================================
	//
	// SIMULATION THREAD
	//
	// =================================================================================================================

	void SimThread::operator()() const
	{
		LOG(Level::DEBUG,
		    "Created simulator thread for transmitter '{}' and receiver '{}'",
		    _trans->getName().c_str(), _recv->getName().c_str());
		try { simulatePair(_trans, _recv, _world); }
		catch (std::exception& ex)
		{
			LOG(Level::ERROR,
			    "First pass thread terminated with unexpected error:\t{}\nSimulator will terminate",
			    ex.what());
			setError();
		}
		decThreads();
	}

	// =================================================================================================================
	//
	// RENDER THREAD
	//
	// =================================================================================================================

	void RenderThread::operator()() const
	{
		LOG(Level::DEBUG, "Created render thread for receiver '{}'",
		    _recv->getName().c_str());
		try { _recv->render(); }
		catch (std::exception& ex)
		{
			LOG(Level::INFO,
			    "Render thread terminated with unexpected error:\t{}Simulator will terminate",
			    ex.what());
			setError();
		}
		decThreads();
	}

	// =================================================================================================================
	//
	// RUN THREADED SIMULATION
	//
	// =================================================================================================================

	void runThreadedSim(const unsigned threadLimit, const World* world)
	{
		std::vector<std::unique_ptr<std::thread>> running;
		LOG(Level::INFO, "Using threaded simulation with {} threads.", threadLimit);

		// Get receivers from the world
		const auto& receivers = world->getReceivers();

		// Run simulation for receiver-transmitter pairs
		runSimForReceiverTransmitterPairs(threadLimit, world, receivers, running);

		// Clear running threads and log responses
		finalizeReceiverResponses(receivers);

		// Run rendering for receivers
		runRenderThreads(threadLimit, receivers, running);
	}
}
