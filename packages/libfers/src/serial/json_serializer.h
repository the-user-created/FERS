// SPDX-License-Identifier: GPL-2-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

/**
 * @file json_serializer.h
 * @brief Provides functions to serialize and deserialize the simulation world to/from JSON.
 *
 * This module is the primary data interchange layer between the C++ core engine and
 * the user interface. It defines the contract for how C++ simulation objects are
 * represented in JSON, enabling the UI to read, modify, and write back the entire
 * simulation state.
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
	 *
	 * This function traverses the `core::World` object model and constructs a JSON
	 * representation. It handles the translation of internal data formats (e.g.,
	 * radians) to a more UI-friendly format (e.g., degrees) and structures the
	 * data in a way that is easy for the frontend to consume.
	 *
	 * @param world The world object to serialize.
	 * @return A nlohmann::json object representing the world.
	 */
	nlohmann::json world_to_json(const core::World& world);

	/**
	 * @brief Deserializes a nlohmann::json object and reconstructs the simulation world.
	 *
	 * This function is the counterpart to `world_to_json`. It clears the existing
	 * world, parses the provided JSON, and rebuilds the entire simulation state
	 * from scratch. This ensures that the C++ core is always in sync with the
	 * state provided by the UI. It also handles re-seeding the random number
	 * generators based on the new scenario data.
	 *
	 * @param j The json object to deserialize.
	 * @param world The world object to populate.
	 * @param masterSeeder A reference to the master random number generator, which will be re-seeded.
	 */
	void json_to_world(const nlohmann::json& j, core::World& world, std::mt19937& masterSeeder);
}
