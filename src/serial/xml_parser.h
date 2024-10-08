/**
* @file xml_parser.h
* @brief Header file for parsing XML configuration files for simulation.
*
* This file declares functions used for parsing XML configuration files that define
* various elements of the simulation, including pulses, antennas, platforms, transmitters,
* receivers, motion paths, and more. It also includes helper functions for parsing and validating
* XML files.
*
* @author David Young
* @date 2024-10-03
*/

#pragma once

#include <string>

namespace core
{
	class World;
}

namespace serial
{
	/**
	* @brief Parses the simulation configuration from an XML file.
	*
	* This function loads an XML file and parses its content, extracting simulation
	* parameters and elements such as pulses, antennas, platforms, and more, and adds them
	* to the provided World object. Optionally, it validates the XML file against its schema.
	*
	* @param filename The path to the XML file to be parsed.
	* @param world A pointer to the World object where parsed data is added.
	* @param validate A flag to indicate whether to validate the XML file against its schema.
	* @throws XmlException if the XML file is invalid or cannot be parsed.
	*/
	void parseSimulation(const std::string& filename, core::World* world, bool validate);
}
