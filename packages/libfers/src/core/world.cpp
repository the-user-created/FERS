// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
 * @file world.cpp
 * @brief Implementation of the World class for the radar simulation environment.
 */

#include "world.h"

#include <iomanip>
#include <sstream>

#include "antenna/antenna_factory.h"
#include "core/sim_events.h"
#include "parameters.h"
#include "radar/radar_obj.h"
#include "signal/radar_signal.h"
#include "timing/prototype_timing.h"

using antenna::Antenna;
using fers_signal::RadarSignal;
using radar::Platform;
using radar::Receiver;
using radar::Target;
using radar::Transmitter;
using timing::PrototypeTiming;

namespace core
{
	void World::add(std::unique_ptr<Platform> plat) noexcept { _platforms.push_back(std::move(plat)); }

	void World::add(std::unique_ptr<Transmitter> trans) noexcept { _transmitters.push_back(std::move(trans)); }

	void World::add(std::unique_ptr<Receiver> recv) noexcept { _receivers.push_back(std::move(recv)); }

	void World::add(std::unique_ptr<Target> target) noexcept { _targets.push_back(std::move(target)); }

	void World::add(std::unique_ptr<RadarSignal> waveform)
	{
		if (_waveforms.contains(waveform->getName()))
		{
			throw std::runtime_error("A waveform with the name " + waveform->getName() + " already exists.");
		}
		_waveforms[waveform->getName()] = std::move(waveform);
	}

	void World::add(std::unique_ptr<Antenna> antenna)
	{
		if (_antennas.contains(antenna->getName()))
		{
			throw std::runtime_error("An antenna with the name " + antenna->getName() + " already exists.");
		}
		_antennas[antenna->getName()] = std::move(antenna);
	}

	void World::add(std::unique_ptr<PrototypeTiming> timing)
	{
		if (_timings.contains(timing->getName()))
		{
			throw std::runtime_error("A timing source with the name " + timing->getName() + " already exists.");
		}
		_timings[timing->getName()] = std::move(timing);
	}

	RadarSignal* World::findWaveform(const std::string& name)
	{
		return _waveforms.contains(name) ? _waveforms[name].get() : nullptr;
	}

	Antenna* World::findAntenna(const std::string& name)
	{
		return _antennas.contains(name) ? _antennas[name].get() : nullptr;
	}

	PrototypeTiming* World::findTiming(const std::string& name)
	{
		return _timings.contains(name) ? _timings[name].get() : nullptr;
	}

	void World::clear() noexcept
	{
		_platforms.clear();
		_transmitters.clear();
		_receivers.clear();
		_targets.clear();
		_waveforms.clear();
		_antennas.clear();
		_timings.clear();
		_event_queue = {};
		_simulation_state = {};
	}

	void World::scheduleInitialEvents()
	{
		for (const auto& transmitter : _transmitters)
		{
			if (transmitter->getMode() == radar::OperationMode::PULSED_MODE)
			{
				// Schedule the first pulse at t=0
				_event_queue.push({0.0, EventType::TX_PULSED_START, transmitter.get()});
			}
			else // CW_MODE
			{
				_event_queue.push({params::startTime(), EventType::TX_CW_START, transmitter.get()});
				_event_queue.push({params::endTime(), EventType::TX_CW_END, transmitter.get()});
			}
		}

		for (const auto& receiver : _receivers)
		{
			if (receiver->getMode() == radar::OperationMode::PULSED_MODE)
			{
				// Schedule the first receive window
				if (const RealType first_window_start = receiver->getWindowStart(0);
					first_window_start < params::endTime())
				{
					_event_queue.push({first_window_start, EventType::RX_PULSED_WINDOW_START, receiver.get()});
				}
			}
			else // CW_MODE
			{
				_event_queue.push({params::startTime(), EventType::RX_CW_START, receiver.get()});
				_event_queue.push({params::endTime(), EventType::RX_CW_END, receiver.get()});
			}
		}
	}

	std::string World::dumpEventQueue() const
	{
		if (_event_queue.empty())
		{
			return "Event Queue is empty.\n";
		}

		std::stringstream ss;
		ss << std::fixed << std::setprecision(6);

		const std::string separator = "--------------------------------------------------------------------";
		const std::string title = "| Event Queue Contents (" + std::to_string(_event_queue.size()) + " events)";

		ss << separator << "\n"
		   << std::left << std::setw(separator.length() - 1) << title << "|\n"
		   << separator << "\n"
		   << "| " << std::left << std::setw(12) << "Timestamp" << " | " << std::setw(21) << "Event Type" << " | "
		   << std::setw(25) << "Source Object" << " |\n"
		   << separator << "\n";

		auto queue_copy = _event_queue;

		while (!queue_copy.empty())
		{
			const auto [timestamp, event_type, source_object] = queue_copy.top();
			queue_copy.pop();

			ss << "| " << std::right << std::setw(12) << timestamp << " | " << std::left << std::setw(21)
			   << toString(event_type) << " | " << std::left << std::setw(25) << source_object->getName() << " |\n";
		}
		ss << separator << "\n";

		return ss.str();
	}
}
