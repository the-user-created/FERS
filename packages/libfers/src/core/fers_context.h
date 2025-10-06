// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

/**
 * @file fers_context.h
 * @brief Internal C++ class that encapsulates the state of a simulation instance.
 */

#pragma once

#include <memory>
#include <random>

#include <libfers/world.h>

/**
 * @class FersContext
 * @brief Manages the lifetime and state of a single FERS simulation scenario.
 *
 * This class serves as the stateful C++ backend for the opaque `fers_context_t`
 * handle exposed by the C-API. Its primary role is to own the `core::World`
 * object, ensuring that the entire simulation scenario persists in memory between
 * API calls from an external client (like the Tauri UI). It also holds the master
 * random number generator to ensure deterministic seeding across the simulation.
 */
class FersContext
{
public:
	/**
	 * @brief Constructs a new simulation context, initializing an empty world.
	 * The master seeder is default-constructed and should be seeded later via
	 * `getMasterSeeder()` after parsing a scenario or receiving configuration.
	 */
	// NOLINTNEXTLINE(cert-msc51-cpp)
	FersContext() :
		_world(std::make_unique<core::World>()) {}

	/**
	 * @brief Retrieves a pointer to the simulation world.
	 * This provides direct access to the in-memory representation of the scenario.
	 * @return A non-owning pointer to the `core::World` object.
	 */
	[[nodiscard]] core::World* getWorld() const noexcept
	{
		return _world.get();
	}

	/**
	 * @brief Retrieves a mutable reference to the master random number seeder.
	 * This is used to seed all random number generators within the simulation
	 * (e.g., for noise models, RCS fluctuations) from a single, controllable source.
	 * @return A reference to the `std::mt19937` engine.
	 */
	[[nodiscard]] std::mt19937& getMasterSeeder() noexcept
	{
		return _master_seeder;
	}

private:
	/// Owns the `core::World` object, which contains all simulation entities.
	std::unique_ptr<core::World> _world;

	/// Master random engine used to seed all other random generators in the simulation.
	std::mt19937 _master_seeder;
};
