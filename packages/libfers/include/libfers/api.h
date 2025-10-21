#ifndef FERS_API_H
#define FERS_API_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief An opaque handle to an in-memory FERS simulation context.
 *
 * This pointer represents a live simulation instance. It is an "opaque" type,
 * meaning its internal structure is hidden from the API consumer. This design
 * is intentional to decouple the client from the C++ implementation details of
 * the simulator core. It ensures that the client code (e.g., in Rust) will not
 * break even if the internal C++ `FersContext` class is changed, maintaining ABI
 * stability across library versions.
 *
 * @note The client is responsible for managing the lifetime of this handle via
 *       `fers_context_create()` and `fers_context_destroy()`.
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
 * @note In a C API, RAII is not available. The caller is responsible for
 *       destroying the returned context using `fers_context_destroy()` to
 *       prevent resource leaks.
 *
 * @return A non-null opaque pointer (handle) to the simulation context on success.
 *         Returns NULL on failure (e.g., out of memory).
 */
fers_context_t* fers_context_create();

/**
 * @brief Destroys a FERS simulation context and releases all associated memory.
 *
 * This function must be called for every context created by `fers_context_create()`
 * to ensure proper cleanup of the underlying C++ objects. Accessing the context
 * handle after calling this function results in undefined behavior.
 *
 * @param context A valid pointer to a `fers_context_t` handle. If context is NULL,
 *                the function performs no action for safety.
 */
void fers_context_destroy(fers_context_t* context);


// --- Scenario Loading & Serialization ---

/**
 * @brief Loads a scenario into the context from a FERS XML file.
 *
 * This is the standard method for initializing a simulation context from a file on
 * disk. It is essential for interoperability with the CLI and legacy workflows that
 * rely on the FERS XML format.
 *
 * @param context A valid `fers_context_t` handle.
 * @param xml_filepath A null-terminated UTF-8 string for the input XML file path.
 * @param validate A boolean (0 or 1) indicating whether to validate the XML
 *                 against the embedded FERS schema. Validation is recommended to
 *                 ensure scenario correctness.
 * @return 0 on success, a non-zero error code on failure. Use
 *         `fers_get_last_error_message()` to retrieve error details.
 */
int fers_load_scenario_from_xml_file(fers_context_t* context, const char* xml_filepath, int validate);

/**
 * @brief Loads a scenario into the context from a FERS XML string.
 *
 * This function provides a way to load a scenario from an in-memory string,
 * avoiding file I/O. It is useful for test harnesses or for UIs that manage
 * scenarios as text content before parsing.
 *
 * @param context A valid `fers_context_t` handle.
 * @param xml_content A null-terminated UTF-8 string containing the FERS scenario in XML format.
 * @param validate A boolean (0 or 1) indicating whether to validate the XML
 *                 against the embedded FERS schema.
 * @return 0 on success, a non-zero error code on failure. Use
 *         `fers_get_last_error_message()` to retrieve error details.
 */
int fers_load_scenario_from_xml_string(fers_context_t* context, const char* xml_content, int validate);

/**
 * @brief Serializes the current simulation scenario into a JSON string.
 *
 * This function is the primary method for the UI to retrieve the full state of
 * the simulation. JSON is used as the interchange format because it is lightweight,
 * human-readable, and natively supported by web technologies, making it trivial
 * to parse and use in the React/TypeScript frontend.
 *
 * @note Memory Management: The returned string is allocated by this library and
 *       its ownership is transferred to the caller. It is crucial to free this
 *       string using `fers_free_string()` to prevent memory leaks.
 *
 * @param context A valid `fers_context_t` handle.
 * @return A dynamically allocated, null-terminated C-string containing the
 *         JSON representation of the scenario. Returns NULL on failure.
 */
char* fers_get_scenario_as_json(fers_context_t* context);

/**
 * @brief Serializes the current simulation scenario into a FERS XML string.
 *
 * This function enables exporting the in-memory state back into the standard FERS
 * XML file format. This is essential for interoperability with legacy tools and
 * for allowing a user to save a scenario that was created or modified in the UI.
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
 * @brief Updates the simulation scenario from a JSON string.
 *
 * This is the primary method for the UI to push its state back to the C++
 * core. It performs a full replacement of the existing scenario. This "replace"
 * strategy was chosen over a more complex "patch" or "diff" approach to simplify
 * state management and ensure the C++ core is always perfectly synchronized with
 * the UI's state representation.
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
 * Because C++ exceptions cannot safely propagate across the FFI boundary into
 * other languages, this function provides the standard C-style error reporting
 * mechanism. The error state is stored in a thread-local variable to ensure that
 * concurrent API calls from different threads do not overwrite each other's error
 * messages. The error is cleared at the start of each fallible API call.
 *
 * @note Memory Management: The returned string's ownership is transferred to the
 *       caller. It MUST be freed using `fers_free_string()` to prevent memory leaks.
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
 * It exists to ensure that the memory deallocation mechanism (`free`) matches the
 * allocation mechanism (`strdup`/`malloc`) used within the C++ library, preventing
 * potential crashes from mismatched allocators across language boundaries.
 *
 * @param str A pointer to the string to be freed. If str is NULL, no action is taken.
 */
void fers_free_string(char* str);


// --- Simulation Execution ---

/**
 * @brief Runs the simulation defined in the provided context.
 *
 * This function is synchronous and will block the calling thread until the
 * simulation is complete. This design keeps the API simple. For use in a
 * responsive UI, it is the responsibility of the caller (e.g., the Tauri
 * backend) to invoke this function on a separate worker thread to avoid
 * freezing the user interface.
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
 * This utility exists to provide a simple, out-of-the-box method for users to
 * validate and visualize the geographic layout and motion paths of their
 * scenarios in common external tools like Google Earth.
 *
 * @param context A valid `fers_context_t` handle containing a loaded scenario.
 * @param output_kml_filepath A null-terminated UTF-8 string for the output KML file path.
 * @return 0 on success, a non-zero error code on failure. Use
 *         `fers_get_last_error_message()` to retrieve error details.
 */
int fers_generate_kml(const fers_context_t* context, const char* output_kml_filepath);

// --- Path Interpolation Utilities ---

/**
 * @brief Defines the interpolation methods available for path generation.
 * This enum provides a language-agnostic way to specify the desired
 * interpolation algorithm when calling the path generation functions.
 */
typedef enum
{
	FERS_INTERP_STATIC,
	FERS_INTERP_LINEAR,
	FERS_INTERP_CUBIC
} fers_interp_type_t;

/**
 * @brief Represents a single waypoint for a motion path.
 * Coordinates are in the scenario's defined coordinate system (e.g., ENU meters).
 */
typedef struct
{
	double time;

	double x;

	double y;

	double z;
} fers_motion_waypoint_t;

/**
 * @brief Represents a single waypoint for a rotation path.
 * Angles are in compass degrees (CW from North).
 */
typedef struct
{
	double time;

	double azimuth_deg;

	double elevation_deg;
} fers_rotation_waypoint_t;

/**
 * @brief Represents a single interpolated point on a motion path.
 */
typedef struct
{
	double x;

	double y;

	double z;
} fers_interpolated_point_t;

/**
 * @brief Represents a single interpolated point on a rotation path.
 * Angles are in compass degrees (CW from North).
 */
typedef struct
{
	double azimuth_deg;

	double elevation_deg;
} fers_interpolated_rotation_point_t;


/**
 * @brief A container for an array of interpolated motion path points.
 * @note The `points` array must be freed using `fers_free_interpolated_motion_path`.
 */
typedef struct
{
	fers_interpolated_point_t* points;

	size_t count;
} fers_interpolated_path_t;

/**
 * @brief A container for an array of interpolated rotation path points.
 * @note The `points` array must be freed using `fers_free_interpolated_rotation_path`.
 */
typedef struct
{
	fers_interpolated_rotation_point_t* points;

	size_t count;
} fers_interpolated_rotation_path_t;


/**
 * @brief Calculates an interpolated motion path from a set of waypoints.
 * This function is a stateless utility that computes the path without needing a
 * full simulation context. It is useful for UI previews.
 *
 * @param waypoints An array of `fers_motion_waypoint_t` structs.
 * @param waypoint_count The number of waypoints in the array.
 * @param interp_type The interpolation algorithm to use.
 * @param num_points The desired number of points in the output interpolated path.
 * @return A pointer to a `fers_interpolated_path_t` struct containing the results.
 *         Returns NULL on failure. The caller owns the returned struct and must
 *         free it with `fers_free_interpolated_motion_path`.
 */
fers_interpolated_path_t* fers_get_interpolated_motion_path(const fers_motion_waypoint_t* waypoints,
                                                            size_t waypoint_count,
                                                            fers_interp_type_t interp_type,
                                                            size_t num_points);

/**
 * @brief Frees the memory allocated for an interpolated motion path.
 * @param path A pointer to the `fers_interpolated_path_t` struct to free.
 */
void fers_free_interpolated_motion_path(fers_interpolated_path_t* path);

/**
 * @brief Calculates an interpolated rotation path from a set of waypoints.
 * This function is a stateless utility for UI previews.
 *
 * @param waypoints An array of `fers_rotation_waypoint_t` structs.
 * @param waypoint_count The number of waypoints in the array.
 * @param interp_type The interpolation algorithm to use (STATIC, LINEAR, CUBIC).
 * @param num_points The desired number of points in the output interpolated path.
 * @return A pointer to a `fers_interpolated_rotation_path_t` struct containing the results.
 *         Returns NULL on failure. The caller owns the returned struct and must
 *         free it with `fers_free_interpolated_rotation_path`.
 */
fers_interpolated_rotation_path_t* fers_get_interpolated_rotation_path(const fers_rotation_waypoint_t* waypoints,
                                                                       size_t waypoint_count,
                                                                       fers_interp_type_t interp_type,
                                                                       size_t num_points);

/**
 * @brief Frees the memory allocated for an interpolated rotation path.
 * @param path A pointer to the `fers_interpolated_rotation_path_t` struct to free.
 */
void fers_free_interpolated_rotation_path(fers_interpolated_rotation_path_t* path);


#ifdef __cplusplus
}
#endif

#endif
