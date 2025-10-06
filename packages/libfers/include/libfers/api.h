// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

/**
* @file api.h
* @brief C-style Foreign Function Interface (FFI) for the libfers core library.
*
* This header defines the public API for creating, managing, and interacting with
* a FERS simulation context from non-C++ languages like Rust. It is designed to be
* a stable ABI boundary, free of C++-specific features like classes or exceptions.
* All functions use standard C types and error handling conventions.
*/

#ifndef FERS_API_H
#define FERS_API_H

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief An opaque handle to an in-memory FERS simulation context.
*
* This pointer represents a live simulation instance, encapsulating the entire
* scenario state. All API functions operate on this handle. It is created by
* `fers_context_create()` and must be destroyed by `fers_context_destroy()`
* to prevent resource leaks.
*/
struct fers_context;

typedef struct fers_context fers_context_t;


// --- Context Lifecycle ---

/**
* @brief Creates a new FERS simulation context.
*
* Allocates and initializes a new, empty simulation context in memory. This
* context serves as the container for a scenario loaded via one of the
* `fers_load_...` or `fers_update_...` functions.
*
* @return A non-null opaque pointer (handle) to the simulation context on success.
*         Returns NULL on failure (e.g., out of memory).
*/
fers_context_t* fers_context_create();

/**
* @brief Destroys a FERS simulation context and releases all associated memory.
*
* This function must be called for every context created by `fers_context_create()`
* to ensure proper cleanup. Accessing the context handle after calling this
* function results in undefined behavior.
*
* @param context A valid pointer to a `fers_context_t` handle. If context is NULL,
*                the function performs no action.
*/
void fers_context_destroy(fers_context_t* context);


// --- Scenario Loading & Serialization ---

/**
* @brief Serializes the current simulation scenario into a JSON string.
*
* This function is the primary method for the UI to retrieve the full state of
* the simulation. It converts the internal C++ object model into a JSON format
* that mirrors the structure used by the frontend.
*
* @note Memory Management: The returned string is dynamically allocated and
*       its ownership is transferred to the caller. It must be freed using
*       `fers_free_string()` to prevent memory leaks.
*
* @param context A valid `fers_context_t` handle.
* @return A dynamically allocated, null-terminated C-string containing the
*         JSON representation of the scenario. Returns NULL on failure.
*/
char* fers_get_scenario_as_json(fers_context_t* context);

/**
* @brief Serializes the current simulation scenario into a FERS XML string.
*
* This function enables exporting the in-memory state (which may have been
* modified by the UI) back into the standard FERS XML file format.
*
* @note Memory Management: The returned string is dynamically allocated and
*       its ownership is transferred to the caller. It must be freed using
*       `fers_free_string()` to prevent memory leaks.
*
* @param context A valid `fers_context_t` handle.
* @return A dynamically allocated, null-terminated C-string containing the
*         XML representation of the scenario. Returns NULL on failure.
*/
char* fers_get_scenario_as_xml(fers_context_t* context);

/**
* @brief Loads a simulation scenario from an XML file into the context.
*
* Parses the specified XML file and populates the simulation world. Any
* existing scenario in the context is cleared first.
*
* @param context A valid `fers_context_t` handle.
* @param xml_filepath A null-terminated UTF-8 string containing the path to the FERS XML file.
* @param validate If non-zero, the XML will be validated against the schema.
* @return 0 on success, a non-zero error code on failure. Use
*         `fers_get_last_error_message()` to retrieve error details.
*/
int fers_load_scenario_from_xml_file(fers_context_t* context, const char* xml_filepath, int validate);

/**
* @brief Loads a simulation scenario from an in-memory XML string.
*
* Parses the provided XML content and populates the simulation world. Any
* existing scenario in the context is cleared first.
*
* @param context A valid `fers_context_t` handle.
* @param xml_content A null-terminated UTF-8 string containing the FERS XML scenario.
* @param validate If non-zero, the XML will be validated against the schema.
* @return 0 on success, a non-zero error code on failure. Use
*         `fers_get_last_error_message()` to retrieve error details.
*/
int fers_load_scenario_from_xml_string(fers_context_t* context, const char* xml_content, int validate);

/**
* @brief Updates the simulation scenario from a JSON string.
*
* This is the primary method for the UI to push its modified state back to the
* C++ core. It deserializes the JSON and rebuilds the internal simulation world.
* Any existing scenario in the context is cleared first.
*
* @param context A valid `fers_context_t` handle.
* @param scenario_json A null-terminated UTF-8 string containing the FERS scenario in JSON format.
* @return 0 on success, a non-zero error code on failure. Use
*         `fers_get_last_error_message()` to retrieve error details.
*/
int fers_update_scenario_from_json(fers_context_t* context, const char* scenario_json);


// --- Error Handling ---

/**
* @brief Retrieves the last error message that occurred on the current thread.
*
* If a libfers API function returns an error, this function can be called
* to get a human-readable description. The error state is thread-local,
* ensuring thread safety, and is cleared at the start of each fallible API call.
*
* @note Memory Management: The returned string is dynamically allocated and
*       its ownership is transferred to the caller. It MUST be freed using
*       `fers_free_string()` to prevent memory leaks.
*
* @return A dynamically allocated, null-terminated C-string containing the last
*         error message, or NULL if no error has occurred.
*/
char* fers_get_last_error_message();

/**
* @brief Frees a string that was allocated and returned by the libfers API.
*
* This function must be used to release memory for any string returned by
* functions like `fers_get_scenario_as_json` or `fers_get_last_error_message`.
* It is the C-side counterpart to the memory management handled by the caller.
*
* @param str A pointer to the string to be freed. If str is NULL, no action is taken.
*/
void fers_free_string(char* str);


// --- Simulation Execution ---

/**
* @brief Runs the simulation defined in the provided context.
*
* This function is synchronous and will block the calling thread until the
* simulation is complete. For use in a responsive UI, it should be invoked
* on a separate worker thread.
*
* @param context A valid `fers_context_t` handle containing a loaded scenario.
* @param user_data An opaque pointer passed for potential future use with callbacks. Currently unused.
* @return 0 on success, a non-zero error code on failure. Use
*         `fers_get_last_error_message()` to retrieve error details.
*/
int fers_run_simulation(fers_context_t* context, void* user_data);


// --- Utility Functions ---

/**
* @brief Generates a KML file for visualizing the scenario in the context.
*
* Creates a KML file for visualization in applications like Google Earth,
* based on the current state of the loaded world.
*
* @param context A valid `fers_context_t` handle containing a loaded scenario.
* @param output_kml_filepath A null-terminated UTF-8 string for the output KML file path.
* @return 0 on success, a non-zero error code on failure. Use
*         `fers_get_last_error_message()` to retrieve error details.
*/
int fers_generate_kml(const fers_context_t* context, const char* output_kml_filepath);

#ifdef __cplusplus
}
#endif

#endif
