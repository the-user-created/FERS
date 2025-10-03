// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

/**
 * @file api.cpp
 * @brief Implementation of the C-style FFI for the libfers core library.
 */

#include <cstring>
#include <string>
#include <libfers/api.h>
#include <libfers/logging.h>
#include <libfers/parameters.h>

#include "core/fers_context.h"
#include "core/sim_threading.h"
#include "core/thread_pool.h"
#include "serial/kml_generator.h"
#include "serial/xml_parser.h"

// The fers_context struct is defined here as an alias for our C++ class.
// This definition is hidden from the C API consumer.
struct fers_context : public FersContext {};

// Thread-local storage for the last error message. Each thread gets its own
// copy, preventing race conditions in a multithreaded FFI consumer.
thread_local std::string last_error_message;

/**
 * @brief Centralized exception handler for the C-API.
 *
 * Logs the exception, stores its message in the thread-local storage,
 * and standardizes the log output.
 * @param e The exception that was caught.
 * @param function_name The name of the API function where the error occurred.
 */
static void handle_api_exception(const std::exception& e, const std::string& function_name)
{
	last_error_message = e.what();
	LOG(logging::Level::ERROR, "API Error in {}: {}", function_name, last_error_message);
}

extern "C" {

fers_context_t* fers_context_create()
{
	last_error_message.clear();
	try { return new fers_context_t(); }
	catch (const std::bad_alloc& e)
	{
		handle_api_exception(e, "fers_create_context");
		return nullptr;
	}
	catch (const std::exception& e)
	{
		handle_api_exception(e, "fers_create_context");
		return nullptr;
	}
}

void fers_context_destroy(fers_context_t* context)
{
	if (!context) { return; }
	delete context;
}

int fers_load_scenario_from_xml_file(fers_context_t* context, const char* xml_filepath, const int validate)
{
	last_error_message.clear();
	if (!context || !xml_filepath)
	{
		last_error_message = "Invalid arguments: context or xml_filepath is NULL.";
		LOG(logging::Level::ERROR, last_error_message);
		return -1;
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
		handle_api_exception(e, "fers_load_scenario_from_xml_file");
		return 1; // Error
	}
}

int fers_load_scenario_from_xml_string(fers_context_t* context, const char* xml_content, const int validate)
{
	last_error_message.clear();
	if (!context || !xml_content)
	{
		last_error_message = "Invalid arguments: context or xml_content is NULL.";
		LOG(logging::Level::ERROR, last_error_message);
		return -1;
	}

	auto* ctx = reinterpret_cast<FersContext*>(context);
	try
	{
		serial::parseSimulationFromString
			(xml_content, ctx->getWorld(), static_cast<bool>(validate),
			ctx->getMasterSeeder()
				);

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
		handle_api_exception(e, "fers_load_scenario_from_xml_string");
		return 1; // Parsing or logic error
	}
}

const char* fers_get_last_error_message()
{
	if (last_error_message.empty())
	{
		return nullptr; // No error to report
	}
	// strdup allocates with malloc, which is part of the C standard ABI.
	// The caller must free this with fers_free_string (which calls free).
	return strdup(last_error_message.c_str());
}

void fers_free_string(char* str) { if (str) { free(str); } }

int fers_run_simulation(fers_context_t* context, void* user_data)
{
	last_error_message.clear();
	if (!context)
	{
		last_error_message = "Invalid context provided to fers_run_simulation.";
		LOG(logging::Level::ERROR, last_error_message);
		return -1;
	}

	auto* ctx = reinterpret_cast<FersContext*>(context);

	try
	{
		pool::ThreadPool pool(params::renderThreads());

		if (params::isCwSimulation()) { core::runThreadedCwSim(ctx->getWorld(), pool); }
		else { core::runThreadedSim(ctx->getWorld(), pool); }

		return 0; // Success
	}
	catch (const std::exception& e)
	{
		handle_api_exception(e, "fers_run_simulation");
		return 1;
	}
}

int fers_generate_kml(const fers_context_t* context, const char* output_kml_filepath)
{
	last_error_message.clear();
	if (!context || !output_kml_filepath)
	{
		last_error_message = "Invalid arguments: context or output_kml_filepath is NULL.";
		LOG(logging::Level::ERROR, last_error_message);
		return -1;
	}

	auto* ctx = reinterpret_cast<const FersContext*>(context);

	try
	{
		if (serial::KmlGenerator::generateKml(*ctx->getWorld(), output_kml_filepath))
		{
			return 0; // Success
		}

		last_error_message = "KML generation failed for an unknown reason.";
		LOG(logging::Level::ERROR, last_error_message);
		return 2; // Generation failed
	}
	catch (const std::exception& e)
	{
		handle_api_exception(e, "fers_generate_kml");
		return 1; // Exception thrown
	}
}

}
