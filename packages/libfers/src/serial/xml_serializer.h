// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

/**
 * @file xml_serializer.h
 * @brief Provides functions to serialize the simulation world to XML.
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
	 * @brief Serializes the entire simulation world into an XML formatted string.
	 * @param world The world object to serialize.
	 * @return A string containing the XML representation of the world.
	 */
	std::string world_to_xml_string(const core::World& world);
}
