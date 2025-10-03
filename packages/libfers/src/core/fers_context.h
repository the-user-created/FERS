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
 * This class is the C++ implementation behind the opaque fers_context handle
 * exposed by the C-FFI. It owns the core::World object and the master random
 * number generator for the simulation instance.
 */
class FersContext {
public:
	/**
	 * @brief Constructs a new simulation context, initializing an empty world.
	 */
	// NOLINTNEXTLINE(cert-msc51-cpp)
	FersContext() :
		_world(std::make_unique<core::World>())
	{

	}

	/**
	 * @brief Retrieves a pointer to the simulation world.
	 * @return A non-owning pointer to the core::World object.
	 */
	[[nodiscard]] core::World* getWorld() const noexcept
	{
		return _world.get();
	}

	/**
	 * @brief Retrieves a mutable reference to the master random number seeder.
	 * @return A reference to the std::mt19937 engine.
	 */
	[[nodiscard]] std::mt19937& getMasterSeeder() noexcept
	{
		return _master_seeder;
	}

private:
	std::unique_ptr<core::World> _world;
	std::mt19937 _master_seeder;
};
