/**
* @file pulse_factory.h
* @brief Interface for loading pulse data into RadarSignal objects.
*
* @authors David Young, Marc Brooker
* @date 2024-09-12
*/

#pragma once

#include <memory>
#include <string>

#include "config.h"

namespace signal
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
	[[nodiscard]] std::unique_ptr<signal::RadarSignal> loadPulseFromFile(
		const std::string& name, const std::string& filename, RealType power, RealType carrierFreq);
}
