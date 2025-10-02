// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

/**
 * @file api.cpp
 * @brief Implementation of the C-style FFI for the libfers core library.
 */

#include <libfers/api.h>
#include <libfers/parameters.h>
#include <cstring>

#include "core/fers_context.h"
#include <libfers/logging.h>
#include "core/sim_threading.h"
#include "core/thread_pool.h"
#include "serial/xml_parser.h"
#include "serial/kml_generator.h"

// The fers_context struct is defined here as an alias for our C++ class.
// This definition is hidden from the C API consumer.
struct fers_context : public FersContext {};

extern "C" {

fers_context* fers_create_context()
{
	try
	{
		return reinterpret_cast<fers_context*>(new FersContext());
	}
	catch (const std::bad_alloc& e)
	{
		LOG(logging::Level::FATAL, "Failed to allocate memory for FersContext: {}", e.what());
		return nullptr;
	}
	catch (const std::exception& e)
	{
		LOG(logging::Level::FATAL, "An unexpected error occurred during context creation: {}", e.what());
		return nullptr;
	}
}

void fers_destroy_context(fers_context* context)
{
	if (!context)
	{
		return;
	}
	delete reinterpret_cast<FersContext*>(context);
}

int fers_load_scenario_from_xml_file(fers_context* context, const char* xml_filepath, int validate)
{
	if (!context || !xml_filepath)
	{
		return -1; // Invalid arguments
	}

	auto* ctx = reinterpret_cast<FersContext*>(context);
	try
	{
		serial::parseSimulation(xml_filepath, ctx->getWorld(), static_cast<bool>(validate), ctx->getMasterSeeder());

		// Initialize the master seeder after parsing
		if (params::params.random_seed)
		{
			LOG(logging::Level::INFO, "Using master seed: {}", *params::params.random_seed);
			ctx->getMasterSeeder().seed(*params::params.random_seed);
		}
		else
		{
			const auto seed = std::random_device{}();
			LOG(logging::Level::INFO, "No master seed provided. Using random_device seed: {}", seed);
			ctx->getMasterSeeder().seed(seed);
		}
		return 0; // Success
	}
	catch (const std::exception& e)
	{
		LOG(logging::Level::ERROR, "Failed to load scenario from file '{}': {}", xml_filepath, e.what());
		return 1; // Error
	}
}

int fers_load_scenario_from_xml_string(fers_context* context, const char* xml_content, int validate)
{
	if (!context || !xml_content)
	{
		return -1; // Invalid arguments
	}

	auto* ctx = reinterpret_cast<FersContext*>(context);
	try
	{
		serial::parseSimulationFromString(xml_content, ctx->getWorld(), static_cast<bool>(validate),
		                                  ctx->getMasterSeeder());

		// Initialize the master seeder after parsing
		if (params::params.random_seed)
		{
			LOG(logging::Level::INFO, "Using master seed: {}", *params::params.random_seed);
			ctx->getMasterSeeder().seed(*params::params.random_seed);
		}
		else
		{
			const auto seed = std::random_device{}();
			LOG(logging::Level::INFO, "No master seed provided. Using random_device seed: {}", seed);
			ctx->getMasterSeeder().seed(seed);
		}

		return 0; // Success
	}
	catch (const std::exception& e)
	{
		LOG(logging::Level::ERROR, "Failed to load scenario from XML string: {}", e.what());
		return 1; // Parsing or logic error
	}
}

void fers_free_string(char* str)
{
	// C standard library's free() must be used as strdup() uses malloc().
	free(str);
}

int fers_run_simulation(fers_context* context, void* user_data)
{
	if (!context)
	{
		return -1; // Invalid context
	}

	auto* ctx = reinterpret_cast<FersContext*>(context);

	try
	{
		// The number of threads is now controlled by a global parameter,
		// which would have been set during XML parsing.
		pool::ThreadPool pool(params::renderThreads());

		if (params::isCwSimulation())
		{
			core::runThreadedCwSim(ctx->getWorld(), pool);
		}
		else
		{
			core::runThreadedSim(ctx->getWorld(), pool);
		}

		return 0; // Success
	}
	catch (const std::exception& e)
	{
		LOG(logging::Level::FATAL, "Simulation encountered an unrecoverable error: {}", e.what());
		return 1;
	}
}

int fers_generate_kml(fers_context* context, const char* output_kml_filepath)
{
	if (!context || !output_kml_filepath)
	{
		return -1; // Invalid arguments
	}

	auto* ctx = reinterpret_cast<FersContext*>(context);

	try
	{
		if (serial::KmlGenerator::generateKml(*ctx->getWorld(), output_kml_filepath))
		{
			return 0; // Success
		}
		LOG(logging::Level::ERROR, "KML generation failed for an unknown reason.");
		return 2; // Generation failed
	}
	catch (const std::exception& e)
	{
		LOG(logging::Level::ERROR, "Exception during KML generation: {}", e.what());
		return 1; // Exception thrown
	}
}


}
