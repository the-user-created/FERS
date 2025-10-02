// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2006-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/**
* @file pulse_factory.h
* @brief Interface for loading pulse data into RadarSignal objects.
*/

#pragma once

#include <memory>
#include <string>

#include <libfers/config.h>

namespace fers_signal
{
	class RadarSignal;
}

namespace serial
{
	/**
	* @brief Loads a radar pulse from a file and returns a RadarSignal object.
	*
	* @param name The name of the radar signal.
	* @param filename The path to the file containing the pulse data.
	* @param power The power of the radar signal in the pulse.
	* @param carrierFreq The carrier frequency of the radar signal.
	* @return A unique pointer to a RadarSignal object loaded with the pulse data.
	* @throws std::runtime_error If the file cannot be opened or the file format is unrecognized.
	*/
	[[nodiscard]] std::unique_ptr<fers_signal::RadarSignal> loadPulseFromFile(
		const std::string& name, const std::string& filename, RealType power, RealType carrierFreq);
}
