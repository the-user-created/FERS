// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

/**
 * @file json_serializer.h
 * @brief Provides functions to serialize the simulation world to JSON.
 */

#pragma once

#include <nlohmann/json.hpp>

namespace core
{
	class World;
}

namespace serial
{
	/**
	 * @brief Serializes the entire simulation world into a nlohmann::json object.
	 * @param world The world object to serialize.
	 * @return A nlohmann::json object representing the world.
	 */
	nlohmann::json world_to_json(const core::World& world);
}
