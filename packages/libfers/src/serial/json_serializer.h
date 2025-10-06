// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

/**
 * @file json_serializer.h
 * @brief Provides functions to serialize the simulation world to JSON.
 */

#pragma once

#include <random>
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

	/**
	 * @brief Deserializes a nlohmann::json object into the simulation world.
	 * @param j The json object to deserialize.
	 * @param world The world object to populate.
	 * @param masterSeeder A reference to the master random number generator.
	 */
	void json_to_world(const nlohmann::json& j, core::World& world, std::mt19937& masterSeeder);
}
