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

#include <execution>
#include <thread>

#include "parameters.h"
#include "world.h"

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

// Flag to set if a thread encounters an error
std::atomic error{false};

namespace
{
	// =================================================================================================================
	//
	// THREAD CLASSES HELPERS
	//
	// =================================================================================================================

	void solveRe(const Transmitter* trans, const Receiver* recv, const Target* targ,
	             const std::chrono::duration<RealType> time, const std::chrono::duration<RealType> length,
	             const RadarSignal* wave, core::ReResults& results)
	{
		LOG(Level::TRACE, "Solving RE for transmitter '{}', receiver '{}' and target '{}'", trans->getName(),
		    recv->getName(), targ->getName());
		// Get initial positions and distance vectors
		const auto transmitter_position = trans->getPosition(time.count());
		const auto receiver_position = recv->getPosition(time.count());
		const auto target_position = targ->getPosition(time.count());

		SVec3 transmitter_to_target_vector{target_position - transmitter_position};
		SVec3 receiver_to_target_vector{target_position - receiver_position};

		const auto transmitter_to_target_distance = transmitter_to_target_vector.length;
		const auto receiver_to_target_distance = receiver_to_target_vector.length;

		constexpr RealType epsilon = std::numeric_limits<RealType>::epsilon();

		if (transmitter_to_target_distance <= epsilon || receiver_to_target_distance <= epsilon)
		{
			throw core::RangeError();
		}

		// Normalize distance vectors
		transmitter_to_target_vector.length = 1;
		receiver_to_target_vector.length = 1;

		results.delay = (transmitter_to_target_distance + receiver_to_target_distance) / params::c();

		const auto rcs = targ->getRcs(transmitter_to_target_vector, receiver_to_target_vector);
		const auto wavelength = params::c() / wave->getCarrier();

		const auto transmitter_gain = trans->
			getGain(transmitter_to_target_vector, trans->getRotation(time.count()), wavelength);
		const auto receiver_gain = recv->getGain(receiver_to_target_vector,
		                                         recv->getRotation(results.delay + time.count()),
		                                         wavelength);

		results.power = transmitter_gain * receiver_gain * rcs / (4 * std::numbers::pi);

		// Propagation loss calculation
		if (!recv->checkFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS))
		{
			const RealType distance_product = transmitter_to_target_distance * receiver_to_target_distance;
			results.power *= std::pow(wavelength, 2) / (std::pow(4 * std::numbers::pi, 2) * std::pow(
				distance_product, 2));
		}

		// Apply multipath factors if applicable
		if (trans->getMultipathDual()) { results.power *= trans->getMultipathFactor(); }
		if (recv->getMultipathDual()) { results.power *= recv->getMultipathFactor(); }

		results.phase = -results.delay * 2 * std::numbers::pi * wave->getCarrier();

		// End position and Doppler calculation
		const auto transvec_end = SVec3(
			targ->getPosition(time.count() + length.count()) - trans->getPosition(time.count() + length.count()));
		const auto recvvec_end = SVec3(
			targ->getPosition(time.count() + length.count()) - recv->getPosition(time.count() + length.count()));

		const RealType rt_end = transvec_end.length;
		const RealType rr_end = recvvec_end.length;

		if (rt_end <= epsilon || rr_end <= epsilon)
		{
			throw std::runtime_error("Target is too close to transmitter or receiver for accurate simulation");
		}

		const RealType v_r = (rr_end - receiver_to_target_distance) / length.count();
		const RealType v_t = (rt_end - transmitter_to_target_distance) / length.count();

		const auto c = params::c();
		results.doppler = std::sqrt((1 + v_r / c) / (1 - v_r / c)) * std::sqrt((1 + v_t / c) / (1 - v_t / c));

		// Set noise temperature
		results.noise_temperature = recv->getNoiseTemperature(recv->getRotation(time.count() + results.delay));
	}

	void solveReDirect(const Transmitter* trans, const Receiver* recv, const std::chrono::duration<RealType> time,
	                   const std::chrono::duration<RealType> length, const RadarSignal* wave, core::ReResults& results)
	{
		// Get positions
		const auto tpos = trans->getPosition(time.count());
		const auto rpos = recv->getPosition(time.count());

		// Calculate distance between transmitter and receiver
		const SVec3 transvec{tpos - rpos};
		const RealType distance = transvec.length;

		constexpr RealType epsilon = std::numeric_limits<RealType>::epsilon();
		constexpr auto pi = std::numbers::pi;

		if (distance <= epsilon) { throw core::RangeError(); }

		// Time delay calculation
		results.delay = distance / params::c();

		// Calculate gains and wavelength
		const RealType wavelength = params::c() / wave->getCarrier();
		const RealType transmitter_gain = trans->getGain(transvec, trans->getRotation(time.count()), wavelength);
		const RealType receiver_gain = recv->getGain(SVec3(rpos - tpos),
		                                             recv->getRotation(time.count() + results.delay),
		                                             wavelength);

		// Power calculation
		results.power = transmitter_gain * receiver_gain * wavelength * wavelength / (4 * pi);

		// Propagation loss
		if (!recv->checkFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS)) { results.power /= 4 * pi * distance * distance; }

		// Doppler shift calculation
		const auto trpos_end = SVec3(
			trans->getPosition(time.count() + length.count()) - recv->getPosition(time.count() + length.count()));
		const RealType r_end = trpos_end.length;
		const RealType doppler_shift = (r_end - distance) / length.count();

		results.doppler = (params::c() + doppler_shift) / (params::c() - doppler_shift);

		// Multipath conditions: if either transmitter or receiver has multipath, zero the power
		if (trans->getMultipathDual() || recv->getMultipathDual()) { results.power = 0; }

		// Phase calculation, ensuring the phase is wrapped within [0, 2π]
		results.phase = std::fmod(results.delay * 2 * pi * wave->getCarrier(), 2 * pi);

		// Set noise temperature
		results.noise_temperature = recv->getNoiseTemperature(recv->getRotation(time.count() + results.delay));
	}

	void addDirect(const Transmitter* trans, Receiver* recv, const TransmitterPulse* signal)
	{
		// Exit early if monostatic and the transmitter is attached to the receiver
		if (trans->isMonostatic() && trans->getAttached() == recv) { return; }

		// Convert time variables to std::chrono for better type safety
		const auto start_time = std::chrono::duration<RealType>(signal->time);
		const auto end_time = start_time + std::chrono::duration<RealType>(signal->wave->getLength());
		const auto sample_time = std::chrono::duration<RealType>(1.0 / params::cwSampleRate());
		const int point_count = static_cast<int>(std::ceil(signal->wave->getLength() / sample_time.count()));

		// Create a response object for this signal
		auto response = std::make_unique<Response>(signal->wave, trans);

		try
		{
			// Iterate through sample points
			for (int i = 0; i <= point_count; ++i)
			{
				// Calculate current time
				const auto current_time = i < point_count ? start_time + i * sample_time : end_time;

				// Compute simulation results
				core::ReResults results{};
				solveReDirect(trans, recv, current_time, sample_time, signal->wave, results);

				// Add interpolation point
				interp::InterpPoint point(results.power, results.delay + current_time.count(), results.delay,
				                          results.doppler, results.phase, results.noise_temperature);
				response->addInterpPoint(point);
			}
		}
		catch (const core::RangeError&)
		{
			throw std::runtime_error("Receiver or Transmitter too close to Target for accurate simulation");
		}

		// Add response to receiver
		recv->addResponse(std::move(response));
	}

	void simulateTarget(const Transmitter* trans, Receiver* recv, const Target* targ,
	                    const TransmitterPulse* signal)
	{
		// Convert time-related values to std::chrono::duration for improved type safety
		const auto start_time = std::chrono::duration<RealType>(signal->time);
		const auto end_time = start_time + std::chrono::duration<RealType>(signal->wave->getLength());
		const auto sample_time = std::chrono::duration<RealType>(1.0 / params::cwSampleRate());
		const int point_count = static_cast<int>(std::ceil(signal->wave->getLength() / sample_time.count()));

		// Create a response object for this signal
		auto response = std::make_unique<Response>(signal->wave, trans);

		try
		{
			// Loop over all time points, including the final one
			for (int i = 0; i <= point_count; ++i)
			{
				// Calculate the current time
				const auto current_time = i < point_count
					                          ? start_time + i * sample_time
					                          : end_time;

				// Simulate the results for each time step
				core::ReResults results{};
				solveRe(trans, recv, targ, current_time, sample_time, signal->wave, results);

				// Add interpolation point to the response
				interp::InterpPoint point(results.power, current_time.count() + results.delay,
				                          results.delay, results.doppler, results.phase,
				                          results.noise_temperature);
				response->addInterpPoint(point);
			}
		}
		catch (const core::RangeError&)
		{
			throw std::runtime_error("Receiver or Transmitter too close to Target for accurate simulation");
		}

		// Add the response to the receiver
		recv->addResponse(std::move(response));
	}

	void simulatePair(const Transmitter* trans, Receiver* recv, const core::World* world)
	{
		constexpr auto flag_nodirect = Receiver::RecvFlag::FLAG_NODIRECT;

		// Use stack allocation instead of dynamic memory for pulse
		TransmitterPulse pulse{};

		// Get the pulse count
		const int pulses = trans->getPulseCount();

		// Loop through all pulses
		for (int i = 0; i < pulses; ++i)
		{
			// Fetch the pulse
			trans->getPulse(&pulse, i);

			// Simulate target interactions for each target in the world
			std::ranges::for_each(world->getTargets(),
			                      [&](const auto& target) { simulateTarget(trans, recv, target.get(), &pulse); });

			// Add direct response unless the flag FLAG_NODIRECT is set
			if (!recv->checkFlag(flag_nodirect)) { addDirect(trans, recv, &pulse); }
		}
	}

	// =================================================================================================================
	//
	// RUN THREADED SIMULATION HELPERS
	//
	// =================================================================================================================

	// Helper function to start a simulation thread
	void startSimThread(const unsigned threadLimit, std::vector<std::jthread>& running,
	                    const std::function<void()>& task)
	{
		++threads;
		while (threads.load() >= static_cast<int>(threadLimit))
		{
			std::this_thread::yield(); // Let other threads run if the limit is reached
		}
		if (error.load()) { throw std::runtime_error("Thread terminated with error. Aborting simulation."); }
		running.emplace_back([task] { task(); });
	}

	// Helper function to run simulation for receiver-transmitter pairs
	void runSimForReceiverTransmitterPairs(const unsigned threadLimit, const core::World* world,
	                                       const std::vector<std::unique_ptr<Receiver>>& receivers,
	                                       std::vector<std::jthread>& running)
	{
		const auto& transmitters = world->getTransmitters();
		for (const auto& receiver : receivers)
		{
			for (const auto& transmitter : transmitters)
			{
				startSimThread(threadLimit, running, [&]
				{
					core::SimThread(transmitter.get(), receiver.get(), world)();
				});
			}
		}
		running.clear(); // jthreads automatically join when destructed
	}
}

namespace core
{
	// =================================================================================================================
	//
	// SIMULATION THREAD
	//
	// =================================================================================================================

	void SimThread::operator()() const
	{
		LOG(Level::DEBUG, "Created simulator thread for transmitter '{}' and receiver '{}'",
		    _trans->getName().c_str(), _recv->getName().c_str());
		try { simulatePair(_trans, _recv, _world); }
		catch (const std::exception& ex)
		{
			LOG(Level::ERROR, "First pass thread terminated with unexpected error:\t{}\nSimulator will terminate",
			    ex.what());
			error.store(true);
		}
		--threads;
	}

	// =================================================================================================================
	//
	// RENDER THREAD
	//
	// =================================================================================================================

	void RenderThread::operator()() const
	{
		LOG(Level::DEBUG, "Created render thread for receiver '{}'", _recv->getName().c_str());
		try { _recv->render(); }
		catch (const std::exception& ex)
		{
			LOG(Level::INFO, "Render thread terminated with unexpected error:\t{}\nSimulator will terminate",
			    ex.what());
			error.store(true);
		}
		--threads;
	}

	// =================================================================================================================
	//
	// RUN THREADED SIMULATION
	//
	// =================================================================================================================

	void runThreadedSim(const unsigned threadLimit, const World* world)
	{
		std::vector<std::jthread> running; // Use jthread to automatically join threads on destruction
		LOG(Level::INFO, "Using threaded simulation with {} threads.", threadLimit);

		// Get receivers from the world
		const auto& receivers = world->getReceivers();

		// Run simulation for receiver-transmitter pairs
		runSimForReceiverTransmitterPairs(threadLimit, world, receivers, running);

		// Clear running threads and log responses
		for (const auto& receiver : receivers)
		{
			LOG(Level::DEBUG, "{} responses added to receiver '{}'", receiver->countResponses(),
			    receiver->getName().c_str());
		}

		// Run rendering for receivers
		for (const auto& receiver : receivers)
		{
			startSimThread(threadLimit, running, [&] { RenderThread(receiver.get())(); });
		}
	}
}
