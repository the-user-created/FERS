// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

/**
 * @file api.h
 * @brief C-style Foreign Function Interface (FFI) for the libfers core library.
 *
 * This header defines the public API for creating, managing, and interacting with
 * a FERS simulation context from non-C++ languages like Rust.
 */

#ifndef FERS_API_H
#define FERS_API_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief An opaque handle to an in-memory FERS simulation context.
 *
 * This pointer represents a live simulation instance. All API functions operate
 * on this handle. It is created by fers_create_context() and must be destroyed
 * by fers_destroy_context() to prevent memory leaks.
 */
struct fers_context;

typedef struct fers_context fers_context_t;

/**
 * @brief Creates a new FERS simulation context.
 *
 * Allocates and initializes a new simulation context in memory. This context
 * will hold the simulation world (scenario) and other relevant state.
 *
 * @return A non-null opaque pointer (handle) to the simulation context on success.
 *         Returns NULL on failure (e.g., out of memory).
 */
fers_context_t* fers_context_create();

/**
 * @brief Destroys a FERS simulation context and releases all associated memory.
 *
 * This function must be called for every context created by fers_create_context()
 * to ensure proper cleanup and to avoid memory leaks. Accessing the context
 * handle after calling this function results in undefined behavior.
 *
 * @param context A valid pointer to a fers_context handle. If context is NULL,
 *                the function does nothing.
 */
void fers_context_destroy(fers_context_t* context);

// --- Scenario Loading & Serialization ---

/**
* @brief Serializes the current simulation scenario into a JSON string.
*
* This function converts the entire state of the simulation world (platforms,
* antennas, etc.) held in the context into a JSON formatted string. This is
* intended for use with a UI to synchronize state.
*
* The returned string is dynamically allocated and must be freed by the caller
* using fers_free_string() to prevent memory leaks.
*
* @param context A valid fers_context handle.
* @return A dynamically allocated, null-terminated C-string containing the
*         JSON representation of the scenario. Returns NULL on failure.
*/
const char* fers_get_scenario_as_json(fers_context_t* context);

/**
 * @brief Loads a simulation scenario from an XML file into the context.
 *
 * This function parses the provided XML file and populates the simulation
 * world within the context. Any existing scenario in the context will be cleared.
 *
 * @param context A valid fers_context handle.
 * @param xml_filepath A null-terminated UTF-8 string containing the path to the FERS XML scenario file.
 * @param validate A boolean flag indicating whether to perform schema validation (true by default).
 * @return 0 on success, a non-zero error code on failure (e.g., file not found, parsing error).
 *         On failure, a detailed error message can be retrieved with fers_get_last_error_message().
 */
int fers_load_scenario_from_xml_file(fers_context_t* context, const char* xml_filepath, int validate);

/**
 * @brief Loads a simulation scenario from an XML string into the context.
 *
 * This function parses the provided XML content and populates the simulation
 * world within the context. Any existing scenario in the context will be cleared.
 *
 * @param context A valid fers_context handle.
 * @param xml_content A null-terminated UTF-8 string containing the FERS XML scenario.
 * @param validate A boolean flag indicating whether to perform schema validation (true by default).
 * @return 0 on success, a non-zero error code on failure (e.g., parsing error).
 *         On failure, a detailed error message can be retrieved with fers_get_last_error_message().
 */
int fers_load_scenario_from_xml_string(fers_context_t* context, const char* xml_content, int validate);

// --- Error Handling ---

/**
 * @brief Retrieves the last error message that occurred on the current thread.
 *
 * If a libfers API function returns an error code, this function can be called
 * immediately to get a human-readable description of the error. The error
 * state is thread-local and is cleared at the beginning of each fallible API call.
 *
 * @return A dynamically allocated, null-terminated C-string containing the last
 *         error message. This string is owned by the caller and MUST be freed
 *         using fers_free_string() to prevent memory leaks.
 *         Returns NULL if no error has occurred since the last successful call
 *         on this thread.
 */
const char* fers_get_last_error_message();

/**
 * @brief Frees a string that was allocated and returned by the libfers API.
 *
 * Some API functions (like fers_get_last_error_message) return dynamically
 * allocated strings. The caller is responsible for freeing these strings using
 * this function to prevent memory leaks.
 *
 * @param str A pointer to the string to be freed. Can be NULL.
 */
void fers_free_string(char* str);

// --- Simulation Execution ---

/**
 * @brief Runs the simulation defined in the provided context.
 *
 * This function is synchronous and will block until the simulation is complete.
 *
 * @param context A valid fers_context handle containing a loaded scenario.
 * @param user_data An opaque pointer that will be passed back to the callback.
 * @return 0 on success, a non-zero error code on failure.
 *         On failure, a detailed error message can be retrieved with fers_get_last_error_message().
 */
int fers_run_simulation(fers_context_t* context, void* user_data);

// --- Utility Functions ---

/**
 * @brief Generates a KML file for visualizing the scenario in the context.
 *
 * This function uses the state of the loaded world within the context to generate
 * a KML file for visualization in applications like Google Earth.
 *
 * @param context A valid fers_context handle containing a loaded scenario.
 * @param output_kml_filepath A null-terminated UTF-8 string for the output KML file path.
 * @return 0 on success, a non-zero error code on failure.
 *         On failure, a detailed error message can be retrieved with fers_get_last_error_message().
 */
int fers_generate_kml(const fers_context_t* context, const char* output_kml_filepath);

#ifdef __cplusplus
}
#endif

#endif
