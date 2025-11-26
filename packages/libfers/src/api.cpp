// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

/**
 * @file api.cpp
 * @brief Implementation of the C-style FFI for the libfers core library.
 *
 * This file provides the C implementations for the functions declared in `api.h`.
 * It acts as the bridge between the C ABI and the C++ core, handling object
 * creation/destruction, exception catching, error reporting, and type casting.
 */

#include <core/logging.h>
#include <core/parameters.h>
#include <cstring>
#include <functional>
#include <libfers/api.h>
#include <math/path.h>
#include <math/rotation_path.h>
#include <nlohmann/json.hpp>
#include <string>

#include "core/fers_context.h"
#include "core/sim_threading.h"
#include "core/thread_pool.h"
#include "serial/json_serializer.h"
#include "serial/kml_generator.h"
#include "serial/xml_parser.h"
#include "serial/xml_serializer.h"

// The fers_context struct is defined here as an alias for our C++ class.
// This allows the C-API to return an opaque pointer, hiding the C++ implementation.
struct fers_context : public FersContext
{
};

// A thread-local error message string ensures that error details from one
// thread's API call do not interfere with another's. This is crucial for a
// thread-safe FFI layer.
thread_local std::string last_error_message;

/**
 * @brief Centralized exception handler for the C-API boundary.
 *
 * This function catches standard C++ exceptions, records their `what()` message
 * into the thread-local error storage, and logs the error. This prevents C++
 * exceptions from propagating across the FFI boundary, which would be undefined behavior.
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
	try
	{
		return new fers_context_t();
	}
	catch (const std::bad_alloc& e)
	{
		handle_api_exception(e, "fers_context_create");
		return nullptr;
	}
	catch (const std::exception& e)
	{
		handle_api_exception(e, "fers_context_create");
		return nullptr;
	}
}

void fers_context_destroy(fers_context_t* context)
{
	if (!context)
	{
		return;
	}
	delete context;
}

// Helper to map C enum to internal C++ enum
static logging::Level map_level(fers_log_level_t level)
{
	switch (level)
	{
	case FERS_LOG_TRACE:
		return logging::Level::TRACE;
	case FERS_LOG_DEBUG:
		return logging::Level::DEBUG;
	case FERS_LOG_INFO:
		return logging::Level::INFO;
	case FERS_LOG_WARNING:
		return logging::Level::WARNING;
	case FERS_LOG_ERROR:
		return logging::Level::ERROR;
	case FERS_LOG_FATAL:
		return logging::Level::FATAL;
	default:
		return logging::Level::INFO;
	}
}

int fers_configure_logging(fers_log_level_t level, const char* log_file_path)
{
	last_error_message.clear();
	try
	{
		logging::logger.setLevel(map_level(level));
		if (log_file_path && *log_file_path)
		{
			auto result = logging::logger.logToFile(log_file_path);
			if (!result)
			{
				last_error_message = result.error();
				return 1;
			}
		}
		return 0;
	}
	catch (const std::exception& e)
	{
		handle_api_exception(e, "fers_configure_logging");
		return 1;
	}
}

void fers_log(fers_log_level_t level, const char* message)
{
	if (!message)
		return;
	// We pass a default source_location because C-API calls don't provide C++ source info
	logging::logger.log(map_level(level), message, std::source_location::current());
}

int fers_set_thread_count(unsigned num_threads)
{
	try
	{
		if (auto res = params::setThreads(num_threads); !res)
		{
			last_error_message = res.error();
			return 1;
		}
		return 0;
	}
	catch (const std::exception& e)
	{
		handle_api_exception(e, "fers_set_thread_count");
		return 1;
	}
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

		// After parsing, seed the master random number generator. This is done
		// to ensure simulation reproducibility. If the scenario specifies a seed,
		// it is used; otherwise, a non-deterministic seed is generated so that
		// subsequent runs are unique by default.
		if (params::params.random_seed)
		{
			LOG(logging::Level::INFO, "Using master seed from scenario file: {}", *params::params.random_seed);
			ctx->getMasterSeeder().seed(*params::params.random_seed);
		}
		else
		{
			const auto seed = std::random_device{}();
			LOG(logging::Level::INFO, "No master seed provided in scenario. Using random_device seed: {}", seed);
			params::params.random_seed = seed;
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
		serial::parseSimulationFromString(xml_content, ctx->getWorld(), static_cast<bool>(validate),
										  ctx->getMasterSeeder());

		// After parsing, seed the master random number generator. This ensures
		// that if the scenario provides a seed, the simulation will be
		// reproducible. If not, a random seed is used to ensure unique runs.
		if (params::params.random_seed)
		{
			LOG(logging::Level::INFO, "Using master seed from scenario string: {}", *params::params.random_seed);
			ctx->getMasterSeeder().seed(*params::params.random_seed);
		}
		else
		{
			const auto seed = std::random_device{}();
			LOG(logging::Level::INFO, "No master seed provided in scenario. Using random_device seed: {}", seed);
			params::params.random_seed = seed;
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

char* fers_get_scenario_as_json(fers_context_t* context)
{
	last_error_message.clear();
	if (!context)
	{
		last_error_message = "Invalid context provided to fers_get_scenario_as_json.";
		LOG(logging::Level::ERROR, last_error_message);
		return nullptr;
	}

	const auto* ctx = reinterpret_cast<FersContext*>(context);
	try
	{
		const nlohmann::json j = serial::world_to_json(*ctx->getWorld());
		const std::string json_str = j.dump(2);
		// A heap-allocated copy of the string is returned. This is necessary
		// to transfer ownership of the memory across the FFI boundary to a
		// client that will free it using `fers_free_string`.
		return strdup(json_str.c_str());
	}
	catch (const std::exception& e)
	{
		handle_api_exception(e, "fers_get_scenario_as_json");
		return nullptr;
	}
}

char* fers_get_scenario_as_xml(fers_context_t* context)
{
	last_error_message.clear();
	if (!context)
	{
		last_error_message = "Invalid context provided to fers_get_scenario_as_xml.";
		LOG(logging::Level::ERROR, last_error_message);
		return nullptr;
	}

	const auto* ctx = reinterpret_cast<FersContext*>(context);
	try
	{
		const std::string xml_str = serial::world_to_xml_string(*ctx->getWorld());
		if (xml_str.empty())
		{
			throw std::runtime_error("XML serialization resulted in an empty string.");
		}
		// `strdup` is used to create a heap-allocated string that can be safely
		// passed across the FFI boundary. The client is responsible for freeing
		// this memory with `fers_free_string`.
		return strdup(xml_str.c_str());
	}
	catch (const std::exception& e)
	{
		handle_api_exception(e, "fers_get_scenario_as_xml");
		return nullptr;
	}
}

int fers_update_scenario_from_json(fers_context_t* context, const char* scenario_json)
{
	last_error_message.clear();
	if (!context || !scenario_json)
	{
		last_error_message = "Invalid arguments: context or scenario_json is NULL.";
		LOG(logging::Level::ERROR, last_error_message);
		return -1;
	}

	auto* ctx = reinterpret_cast<FersContext*>(context);
	try
	{
		const nlohmann::json j = nlohmann::json::parse(scenario_json);
		serial::json_to_world(j, *ctx->getWorld(), ctx->getMasterSeeder());

		return 0; // Success
	}
	catch (const nlohmann::json::exception& e)
	{
		// A specific catch block for JSON errors is used to provide more
		// detailed feedback to the client (e.g., the UI), which can help
		// developers diagnose schema or data format issues more easily.
		last_error_message = "JSON parsing/deserialization error: " + std::string(e.what());
		LOG(logging::Level::ERROR, "API Error in {}: {}", "fers_update_scenario_from_json", last_error_message);
		return 2; // JSON error
	}
	catch (const std::exception& e)
	{
		handle_api_exception(e, "fers_update_scenario_from_json");
		return 1; // Generic error
	}
}

char* fers_get_last_error_message()
{
	if (last_error_message.empty())
	{
		return nullptr; // No error to report
	}
	// `strdup` allocates with `malloc`, which is part of the C standard ABI,
	// making it safe to transfer ownership across the FFI boundary. The caller
	// must then free this memory using `fers_free_string`.
	return strdup(last_error_message.c_str());
}

void fers_free_string(char* str)
{
	if (str)
	{
		free(str);
	}
}

int fers_run_simulation(fers_context_t* context, fers_progress_callback_t callback, void* user_data)
{
	last_error_message.clear();
	if (!context)
	{
		last_error_message = "Invalid context provided to fers_run_simulation.";
		LOG(logging::Level::ERROR, last_error_message);
		return -1;
	}

	auto* ctx = reinterpret_cast<FersContext*>(context);

	// Wrap the C-style callback in a std::function for easier use in C++.
	// This also handles the case where the callback is null.
	std::function<void(const std::string&, int, int)> progress_fn;
	if (callback)
	{
		progress_fn = [callback, user_data](const std::string& msg, const int current, const int total)
		{ callback(msg.c_str(), current, total, user_data); };
	}

	try
	{
		pool::ThreadPool pool(params::renderThreads());

		core::runEventDrivenSim(ctx->getWorld(), pool, progress_fn);

		return 0;
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

// --- Helper to convert C-API enum to C++ enum ---
math::Path::InterpType to_cpp_interp_type(const fers_interp_type_t type)
{
	switch (type)
	{
	case FERS_INTERP_LINEAR:
		return math::Path::InterpType::INTERP_LINEAR;
	case FERS_INTERP_CUBIC:
		return math::Path::InterpType::INTERP_CUBIC;
	case FERS_INTERP_STATIC:
	default:
		return math::Path::InterpType::INTERP_STATIC;
	}
}

math::RotationPath::InterpType to_cpp_rot_interp_type(const fers_interp_type_t type)
{
	switch (type)
	{
	case FERS_INTERP_LINEAR:
		return math::RotationPath::InterpType::INTERP_LINEAR;
	case FERS_INTERP_CUBIC:
		return math::RotationPath::InterpType::INTERP_CUBIC;
	case FERS_INTERP_STATIC:
	default:
		return math::RotationPath::InterpType::INTERP_STATIC;
	}
}


fers_interpolated_path_t* fers_get_interpolated_motion_path(const fers_motion_waypoint_t* waypoints,
															const size_t waypoint_count,
															const fers_interp_type_t interp_type,
															const size_t num_points)
{
	last_error_message.clear();
	if (!waypoints || waypoint_count == 0 || num_points == 0)
	{
		last_error_message = "Invalid arguments: waypoints cannot be null and counts must be > 0.";
		LOG(logging::Level::ERROR, last_error_message);
		return nullptr;
	}
	if (interp_type == FERS_INTERP_CUBIC && waypoint_count < 2)
	{
		last_error_message = "Cubic interpolation requires at least 2 waypoints.";
		LOG(logging::Level::ERROR, last_error_message);
		return nullptr;
	}

	try
	{
		math::Path path;
		path.setInterp(to_cpp_interp_type(interp_type));

		for (size_t i = 0; i < waypoint_count; ++i)
		{
			math::Coord c;
			c.t = waypoints[i].time;
			c.pos.x = waypoints[i].x;
			c.pos.y = waypoints[i].y;
			c.pos.z = waypoints[i].z;
			path.addCoord(c);
		}

		path.finalize();

		auto* result_path = new fers_interpolated_path_t();
		result_path->points = new fers_interpolated_point_t[num_points];
		result_path->count = num_points;

		const double start_time = waypoints[0].time;
		const double end_time = waypoints[waypoint_count - 1].time;
		const double duration = end_time - start_time;

		// Handle static case separately
		if (waypoint_count < 2 || duration <= 0)
		{
			const math::Vec3 pos = path.getPosition(start_time);
			for (size_t i = 0; i < num_points; ++i)
			{
				result_path->points[i] = {pos.x, pos.y, pos.z};
			}
			return result_path;
		}

		const double time_step = duration / (num_points > 1 ? num_points - 1 : 1);

		for (size_t i = 0; i < num_points; ++i)
		{
			const double t = start_time + i * time_step;
			const math::Vec3 pos = path.getPosition(t);
			result_path->points[i] = {pos.x, pos.y, pos.z};
		}

		return result_path;
	}
	catch (const std::exception& e)
	{
		handle_api_exception(e, "fers_get_interpolated_motion_path");
		return nullptr;
	}
}

void fers_free_interpolated_motion_path(fers_interpolated_path_t* path)
{
	if (path)
	{
		delete[] path->points;
		delete path;
	}
}

fers_interpolated_rotation_path_t* fers_get_interpolated_rotation_path(const fers_rotation_waypoint_t* waypoints,
																	   const size_t waypoint_count,
																	   const fers_interp_type_t interp_type,
																	   const size_t num_points)
{
	last_error_message.clear();
	if (!waypoints || waypoint_count == 0 || num_points == 0)
	{
		last_error_message = "Invalid arguments: waypoints cannot be null and counts must be > 0.";
		LOG(logging::Level::ERROR, last_error_message);
		return nullptr;
	}
	if (interp_type == FERS_INTERP_CUBIC && waypoint_count < 2)
	{
		last_error_message = "Cubic interpolation requires at least 2 waypoints.";
		LOG(logging::Level::ERROR, last_error_message);
		return nullptr;
	}

	try
	{
		math::RotationPath path;
		path.setInterp(to_cpp_rot_interp_type(interp_type));

		for (size_t i = 0; i < waypoint_count; ++i)
		{
			const RealType az_deg = waypoints[i].azimuth_deg;
			const RealType el_deg = waypoints[i].elevation_deg;

			// Convert from compass degrees (from C-API) to internal mathematical radians
			const RealType az_rad = (90.0 - az_deg) * (PI / 180.0);
			const RealType el_rad = el_deg * (PI / 180.0);

			path.addCoord({az_rad, el_rad, waypoints[i].time});
		}

		path.finalize();

		auto* result_path = new fers_interpolated_rotation_path_t();
		result_path->points = new fers_interpolated_rotation_point_t[num_points];
		result_path->count = num_points;

		const double start_time = waypoints[0].time;
		const double end_time = waypoints[waypoint_count - 1].time;
		const double duration = end_time - start_time;

		// Handle static case separately
		if (waypoint_count < 2 || duration <= 0)
		{
			const math::SVec3 rot = path.getPosition(start_time);
			// Convert back to compass degrees for output without normalization
			const RealType az_deg = 90.0 - rot.azimuth * 180.0 / PI;
			const RealType el_deg = rot.elevation * 180.0 / PI;
			for (size_t i = 0; i < num_points; ++i)
			{
				result_path->points[i] = {az_deg, el_deg};
			}
			return result_path;
		}

		const double time_step = duration / (num_points > 1 ? num_points - 1 : 1);

		for (size_t i = 0; i < num_points; ++i)
		{
			const double t = start_time + i * time_step;
			const math::SVec3 rot = path.getPosition(t);

			// Convert from internal mathematical radians back to compass degrees for C-API output
			// We do NOT normalize to [0, 360) to preserve winding/negative angles for the UI.
			const RealType az_deg = 90.0 - rot.azimuth * 180.0 / PI;
			const RealType el_deg = rot.elevation * 180.0 / PI;

			result_path->points[i] = {az_deg, el_deg};
		}

		return result_path;
	}
	catch (const std::exception& e)
	{
		handle_api_exception(e, "fers_get_interpolated_rotation_path");
		return nullptr;
	}
}

void fers_free_interpolated_rotation_path(fers_interpolated_rotation_path_t* path)
{
	if (path)
	{
		delete[] path->points;
		delete path;
	}
}
}
