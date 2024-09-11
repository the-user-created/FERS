//rssim.cpp
//Functions which perform the actual simulations
//Marc Brooker mbrooker@rrsg.ee.uct.ac.za
//30 May 2006

#include "rssim.h"

#include <cmath>
#include <limits>
#include <stdexcept>

#include "rsantenna.h"
#include "rsdebug.h"
#include "rsnoise.h"
#include "rsparameters.h"
#include "rsradar.h"
#include "rsresponse.h"
#include "rstarget.h"
#include "rsworld.h"

using namespace rs;

namespace
{
	/// Results of solving the bistatic radar equation and friends
	struct ReResults
	{
		RS_FLOAT power;
		RS_FLOAT delay;
		RS_FLOAT doppler;
		RS_FLOAT phase;
		RS_FLOAT noise_temperature;
	};

	/// Class for range errors in RE calculations
	class RangeError
	{
	};

	///Solve the radar equation for a given set of parameters
	void solveRe(const Transmitter* trans, const Receiver* recv, const Target* targ, const RS_FLOAT time,
	             const RS_FLOAT length, const RadarSignal* wave, ReResults& results)
	{
		//Get the positions in space of the three objects
		const Vec3 transmitter_position = trans->getPosition(time);
		const Vec3 receiver_position = recv->getPosition(time);
		const Vec3 target_position = targ->getPosition(time);
		auto transmitter_to_target_vector = SVec3(target_position - transmitter_position);
		auto receiver_to_target_vector = SVec3(target_position - receiver_position);
		//Calculate the distances
		const RS_FLOAT transmitter_to_target_distance = transmitter_to_target_vector.length;
		const RS_FLOAT receiver_to_target_distance = receiver_to_target_vector.length;
		// From here on, transvec and recvvec need to be normalized
		transmitter_to_target_vector.length = 1;
		receiver_to_target_vector.length = 1;
		//Sanity check Rt and Rr and throw an exception if they are too small
		if (transmitter_to_target_distance <= std::numeric_limits<RS_FLOAT>::epsilon() || receiver_to_target_distance <= std::numeric_limits<RS_FLOAT>::epsilon())
		{
			throw RangeError();
		}
		//Step 1, calculate the delay (in seconds) experienced by the pulse
		//See "Delay Equation" in doc/equations/equations.tex
		results.delay = (transmitter_to_target_distance + receiver_to_target_distance) / RsParameters::c();
		//rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Calculated delay as %fs, %fm\n", results.delay, results.delay * rsParameters::c());
		//Get the RCS
		const RS_FLOAT rcs = targ->getRcs(transmitter_to_target_vector, receiver_to_target_vector);
		//Get the wavelength
		const RS_FLOAT wavelength = RsParameters::c() / wave->getCarrier();
		//Get the system antenna gains (which include loss factors)
		const RS_FLOAT transmitter_gain = trans->getGain(transmitter_to_target_vector, trans->getRotation(time), wavelength);
		const RS_FLOAT receiver_gain = recv->getGain(receiver_to_target_vector, recv->getRotation(results.delay + time), wavelength);
		//  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Gt: %e Gr: %e\n", Gt, Gr);
		//Step 2, calculate the received power using the narrowband bistatic radar equation
		//See "Bistatic Narrowband Radar Equation" in doc/equations/equations.tex
		results.power = transmitter_gain * receiver_gain * rcs / (4 * M_PI);
		if (!recv->checkFlag(Receiver::FLAG_NOPROPLOSS))
		{
			results.power *= wavelength * wavelength / (pow(4 * M_PI, 2) * transmitter_to_target_distance * transmitter_to_target_distance * receiver_to_target_distance * receiver_to_target_distance);
		}
		//   rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Pr: %2.9e Rt: %e Rr: %e Gt: %e Gr: %e RCS: %e Wl %e\n", results.power, Rt, Rr, Gt, Gr, RCS, Wl);
		// If the transmitter and/or receiver are multipath duals, multiply by the loss factor
		if (trans->isMultipathDual())
		{
			results.power *= trans->multipathDualFactor();
		}
		if (recv->isMultipathDual())
		{
			results.power *= trans->multipathDualFactor();
		}
		//Step 3, calculate phase shift
		//See "Phase Delay Equation" in doc/equations/equations.tex
		// results.phase = fmod(results.delay*2*M_PI*wave->GetCarrier(), 2*M_PI);
		results.phase = -results.delay * 2 * M_PI * wave->getCarrier();
		//Step 4, calculate doppler shift
		//Calculate positions at the end of the pulse
		// TODO: These need to be more descriptive
		const Vec3 trpos_end = trans->getPosition(time + length);
		const Vec3 repos_end = recv->getPosition(time + length);
		const Vec3 tapos_end = targ->getPosition(time + length);
		const auto transvec_end = SVec3(tapos_end - trpos_end);
		const auto recvvec_end = SVec3(tapos_end - repos_end);
		const RS_FLOAT rt_end = transvec_end.length;
		const RS_FLOAT rr_end = recvvec_end.length;
		//Sanity check Rt_end and Rr_end and throw an exception if they are too small
		if (rt_end < std::numeric_limits<RS_FLOAT>::epsilon() || rr_end < std::numeric_limits<RS_FLOAT>::epsilon())
		{
			throw std::runtime_error("Target is too close to transmitter or receiver for accurate simulation");
		}
		//Doppler shift equation
		//See "Bistatic Doppler Equation" in doc/equations/equations.tex
		const RS_FLOAT v_r = (rr_end - receiver_to_target_distance) / length;
		const RS_FLOAT v_t = (rt_end - transmitter_to_target_distance) / length;
		results.doppler = std::sqrt((1 + v_r / RsParameters::c()) / (1 - v_r / RsParameters::c())) * std::sqrt(
			(1 + v_t / RsParameters::c()) / (1 - v_t / RsParameters::c()));
		//Step 5, calculate system noise temperature
		//We only use the receive antenna noise temperature for now
		results.noise_temperature = recv->getNoiseTemperature(recv->getRotation(time + results.delay));
	}

	//Perform the first stage of CW simulation calculations for the specified pulse and target
	void simulateTarget(const Transmitter* trans, Receiver* recv, const Target* targ,
	                    const TransmitterPulse* signal)
	{
		//Get the simulation start and end time
		const RS_FLOAT start_time = signal->time;
		const RS_FLOAT end_time = signal->time + signal->wave->getLength();
		//Calculate the number of interpolation points we need to add
		const RS_FLOAT sample_time = 1.0 / RsParameters::cwSampleRate();
		const RS_FLOAT point_count = std::ceil(signal->wave->getLength() / sample_time);
		//Create the response
		auto* response = new Response(signal->wave, trans);
		try
		{
			//Loop through and add interpolation points
			for (int i = 0; i < point_count; i++)
			{
				const RS_FLOAT stime = i * sample_time + start_time; //Time of the start of the sample
				ReResults results{};
				solveRe(trans, recv, targ, stime, sample_time, signal->wave, results);
				InterpPoint point(results.power, stime + results.delay, results.delay, results.doppler, results.phase,
				                  results.noise_temperature);
				response->addInterpPoint(point);
				//rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Added interpolation point with delay: %gs, %fm\n", results.delay, results.delay * rsParameters::c());
				//rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "stime + results.delay = : %gs\n", stime + results.delay);
			}
			//Add one more point at the end
			ReResults results{};
			solveRe(trans, recv, targ, end_time, sample_time, signal->wave, results);
			const InterpPoint point(results.power, end_time + results.delay, results.delay, results.doppler,
			                        results.phase,
			                        results.noise_temperature);
			response->addInterpPoint(point);
		}
		catch ([[maybe_unused]] RangeError& re)
		{
			throw std::runtime_error("Receiver or Transmitter too close to Target for accurate simulation");
		}
		//Add the response to the receiver
		recv->addResponse(response);
	}

	/// Solve the Radar Equation and friends (doppler, phase, delay) for direct transmission
	void solveReDirect(const Transmitter* trans, const Receiver* recv, const RS_FLOAT time, const RS_FLOAT length,
	                   const RadarSignal* wave, ReResults& results)
	{
		//Calculate the vectors to and from the transmitter
		// TODO: These need to be more descriptive
		const Vec3 tpos = trans->getPosition(time);
		const Vec3 rpos = recv->getPosition(time);
		auto transvec = SVec3(tpos - rpos);
		auto recvvec = SVec3(rpos - tpos);
		//Calculate the range
		const RS_FLOAT r = transvec.length;
		//Normalize transvec and recvvec for angle calculations
		transvec.length = 1;
		recvvec.length = 1;
		//If the two antennas are not in the same position, this can be calculated
		if (r > std::numeric_limits<RS_FLOAT>::epsilon())
		{
			//Step 1: Calculate the delay
			//See "Delay Equation" in doc/equations/equations.tex
			results.delay = r / RsParameters::c();

			//Calculate the wavelength
			const RS_FLOAT wl = RsParameters::c() / wave->getCarrier();
			//Get the antenna gains
			const RS_FLOAT gt = trans->getGain(transvec, trans->getRotation(time), wl);
			const RS_FLOAT gr = recv->getGain(recvvec, recv->getRotation(time + results.delay), wl);
			//Step 2: Calculate the received power
			//See "One Way Radar Equation" in doc/equations/equations.tex
			results.power = gt * gr * wl * wl / (4 * M_PI);
			if (!recv->checkFlag(Receiver::FLAG_NOPROPLOSS))
			{
				results.power *= 1 / (4 * M_PI * pow(r, 2));
			}
			//  rsDebug::printf(rsDebug::RS_VERY_VERBOSE, "Pr: %2.9e R: %e Gt: %e Gr: %e Wl %e\n", results.power, R, Gt, Gr, Wl);
			//Step 3: Calculate the doppler shift (if one of the antennas is moving)
			// TODO: These need to be more descriptive
			const Vec3 tpos_end = trans->getPosition(time + length);
			const Vec3 rpos_end = recv->getPosition(time + length);
			const Vec3 trpos_end = tpos_end - rpos_end;
			const RS_FLOAT r_end = trpos_end.length();
			//Calculate the Doppler shift
			//See "Monostatic Doppler Equation" in doc/equations/equations.tex
			const RS_FLOAT vdoppler = (r_end - r) / length;
			results.doppler = (RsParameters::c() + vdoppler) / (RsParameters::c() - vdoppler);
			// For the moment, we don't handle direct paths for multipath duals, until we can do it correctly
			if (trans->isMultipathDual())
			{
				results.power = 0;
			}
			if (recv->isMultipathDual())
			{
				results.power = 0;
			}

			//Step 4, calculate phase shift
			//See "Phase Delay Equation" in doc/equations/equations.tex
			results.phase = fmod(results.delay * 2 * M_PI * wave->getCarrier(), 2 * M_PI);

			//Step 5, calculate noise temperature
			results.noise_temperature = recv->getNoiseTemperature(recv->getRotation(time + results.delay));
		}
		else
		{
			//If the receiver and transmitter are too close, throw a range error
			throw RangeError();
		}
	}

	/// Model the pulse which is received directly by a receiver from a CW transmitter
	void addDirect(const Transmitter* trans, Receiver* recv, const TransmitterPulse* signal)
	{
		//If receiver and transmitter share the same antenna - there can't be a direct pulse
		if (trans->isMonostatic() && trans->getAttached() == recv)
		{
			return;
		}
		//Get the simulation start and end time
		const RS_FLOAT start_time = signal->time;
		const RS_FLOAT end_time = signal->time + signal->wave->getLength();
		//Calculate the number of interpolation points we need to add
		const RS_FLOAT sample_time = 1.0 / RsParameters::cwSampleRate();
		const RS_FLOAT point_count = std::ceil(signal->wave->getLength() / sample_time);
		//Create the CW response
		auto* response = new Response(signal->wave, trans);
		try
		{
			//Loop through and add interpolation points
			for (int i = 0; i < point_count; i++)
			{
				const RS_FLOAT stime = i * sample_time + start_time;
				ReResults results{};
				solveReDirect(trans, recv, stime, sample_time, signal->wave, results);
				InterpPoint point(results.power, results.delay + stime, results.delay, results.doppler, results.phase,
				                  results.noise_temperature);
				response->addInterpPoint(point);
			}
			//Add one more point at the end
			ReResults results{};
			solveReDirect(trans, recv, end_time, sample_time, signal->wave, results);
			const InterpPoint point(results.power, results.delay + end_time, results.delay, results.doppler,
			                        results.phase,
			                        results.noise_temperature);
			response->addInterpPoint(point);
		}
		catch ([[maybe_unused]] RangeError& re)
		{
			throw std::runtime_error("Receiver or Transmitter too close to Target for accurate simulation");
		}
		//Add the response to the receiver
		recv->addResponse(response);
	}
}

//Simulate a transmitter-receiver pair with a pulsed transmission
void rs::simulatePair(const Transmitter* trans, Receiver* recv, const World* world)
{
	//Get the number of pulses
	const int pulses = trans->getPulseCount();
	//Build a pulse
	auto* pulse = new TransmitterPulse();
	rs_debug::printf(rs_debug::RS_VERY_VERBOSE, "%d\n", pulses);
	//Loop throught the pulses
	for (int i = 0; i < pulses; i++)
	{
		trans->getPulse(pulse, i);
		for (auto _target : world->_targets)
		{
			simulateTarget(trans, recv, _target, pulse);
		}

		// Check if direct pulses are being considered for this receiver
		if (!recv->checkFlag(Receiver::FLAG_NODIRECT))
		{
			//Add the direct pulses
			addDirect(trans, recv, pulse);
		}
	}
	delete pulse;
}
