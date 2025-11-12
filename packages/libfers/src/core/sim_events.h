// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
 * @file sim_events.h
 * @brief Defines the core structures for the event-driven simulation engine.
 */

#pragma once

#include <libfers/config.h>

namespace radar
{
	class Radar;
}

namespace core
{
	/**
	 * @enum EventType
	 * @brief Enumerates the types of events that can occur in the simulation.
	 */
	enum class EventType
	{
		TxPulsedStart, ///< A pulsed transmitter begins emitting a pulse.
		RxPulsedWindowStart, ///< A pulsed receiver opens its listening window.
		RxPulsedWindowEnd, ///< A pulsed receiver closes its listening window.
		TxCwStart, ///< A continuous-wave transmitter starts transmitting.
		TxCwEnd, ///< A continuous-wave transmitter stops transmitting.
		RxCwStart, ///< A continuous-wave receiver starts listening.
		RxCwEnd, ///< A continuous-wave receiver stops listening.
	};

	/**
	 * @struct Event
	 * @brief Represents a single event in the simulation's time-ordered queue.
	 */
	struct Event
	{
		RealType timestamp; ///< The simulation time at which the event occurs.
		EventType type; ///< The type of the event.
		radar::Radar* source_object; ///< Pointer to the object that generated the event.
	};

	/**
	 * @struct EventComparator
	 * @brief A custom comparator for the event priority queue.
	 *
	 * This comparator creates a min-heap, ensuring that the event with the
	 * smallest timestamp is always at the top of the queue.
	 */
	struct EventComparator
	{
		/**
		 * @brief Compares two events based on their timestamps.
		 * @param a The first event.
		 * @param b The second event.
		 * @return True if event 'a' should occur after event 'b'.
		 */
		bool operator()(const Event& a, const Event& b) const noexcept
		{
			return a.timestamp > b.timestamp;
		}
	};

	/**
	 * @brief Converts an EventType enum to its string representation.
	 * @param type The event type.
	 * @return A string representing the event type.
	 */
	inline std::string toString(const EventType type)
	{
		switch (type)
		{
		case EventType::TxPulsedStart:
			return "TxPulsedStart";
		case EventType::RxPulsedWindowStart:
			return "RxPulsedWindowStart";
		case EventType::RxPulsedWindowEnd:
			return "RxPulsedWindowEnd";
		case EventType::TxCwStart:
			return "TxCwStart";
		case EventType::TxCwEnd:
			return "TxCwEnd";
		case EventType::RxCwStart:
			return "RxCwStart";
		case EventType::RxCwEnd:
			return "RxCwEnd";
		default:
			return "UnknownEvent";
		}
	}
}
