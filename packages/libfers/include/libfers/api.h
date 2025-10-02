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

typedef struct fers_context fers_context;

/**
 * @brief Creates a new FERS simulation context.
 *
 * Allocates and initializes a new simulation context in memory. This context
 * will hold the simulation world (scenario) and other relevant state.
 *
 * @return A non-null opaque pointer (handle) to the simulation context on success.
 *         Returns NULL on failure (e.g., out of memory).
 */
fers_context* fers_create_context();

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
void fers_destroy_context(fers_context* context);

// --- Scenario Loading & Serialization ---

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
 */
int fers_load_scenario_from_xml_file(fers_context* context, const char* xml_filepath, int validate);

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
 */
int fers_load_scenario_from_xml_string(fers_context* context, const char* xml_content, int validate);

/**
 * @brief Frees a string that was allocated and returned by the libfers API.
 *
 * Some API functions return dynamically allocated strings. The caller is
 * responsible for freeing these strings using this function to prevent memory leaks.
 *
 * @param str A pointer to the string to be freed. Can be NULL.
 */
void fers_free_string(char* str);


// --- Simulation Execution ---

/**
 * @brief A callback function pointer for reporting simulation progress.
 *
 * @param progress A value between 0.0 and 1.0 indicating the simulation progress.
 * @param status_message A descriptive message about the current simulation phase.
 * @param user_data An opaque pointer passed from the fers_run_simulation call,
 *                  useful for identifying context in the calling language.
 */
typedef void (*FersProgressCallback)(double progress, const char* status_message, void* user_data);

/**
 * @brief Runs the simulation defined in the provided context.
 *
 * This function is synchronous and will block until the simulation is complete.
 * The provided callback will be invoked periodically to report progress.
 *
 * @param context A valid fers_context handle containing a loaded scenario.
 * @param callback A function pointer for progress reporting. Can be NULL if no
 *                 progress reporting is needed.
 * @param user_data An opaque pointer that will be passed back to the callback.
 * @return 0 on success, a non-zero error code on failure.
 */
int fers_run_simulation(fers_context* context, FersProgressCallback callback, void* user_data);

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
 */
int fers_generate_kml(fers_context* context, const char* output_kml_filepath);

#ifdef __cplusplus
}
#endif

#endif
