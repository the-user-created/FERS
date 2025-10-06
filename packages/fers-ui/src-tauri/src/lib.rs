// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

//! # Tauri Application Library Entry Point
//!
//! This module provides the main entry point for the FERS Tauri desktop application.
//! It bridges the Rust/Tauri frontend with the C++ `libfers` simulation engine via
//! a Foreign Function Interface (FFI).
//!
//! ## Architecture Overview
//!
//! The application follows a three-layer architecture:
//!
//! 1. **Frontend (TypeScript/React)**: Provides the user interface for scenario
//!    editing and visualization.
//! 2. **Middle Layer (Rust/Tauri)**: This module. Exposes Tauri commands that the
//!    frontend can invoke, and manages the simulation state via a thread-safe wrapper.
//! 3. **Backend (C++ via FFI)**: The `libfers` library that performs the actual
//!    simulation computations, parsing, and serialization.
//!
//! ## Thread Safety
//!
//! The `FersContext` (which wraps the C++ simulation state) is protected by a `Mutex`
//! and stored in Tauri's managed state. This ensures that concurrent calls from the
//! UI are serialized, preventing data races on the non-thread-safe C++ object.
//!
//! ## Tauri Commands
//!
//! All functions annotated with `#[tauri::command]` are exposed to the frontend via
//! Tauri's IPC mechanism. They can be invoked asynchronously from JavaScript/TypeScript.

mod fers_api;

use std::sync::Mutex;
use tauri::State;

/// Type alias for the managed Tauri state that holds the simulation context.
///
/// The `FersContext` is wrapped in a `Mutex` to ensure thread-safe access, as Tauri
/// may invoke commands from multiple threads concurrently. This alias simplifies
/// the function signatures of Tauri commands.
type FersState = Mutex<fers_api::FersContext>;

// --- Tauri Commands ---

/// Loads a FERS scenario from an XML file into the simulation context.
///
/// This command replaces any existing in-memory scenario with the one parsed from
/// the specified file. The file path is provided by the user via the frontend dialog.
///
/// # Parameters
///
/// * `filepath` - The absolute or relative path to the FERS XML scenario file.
/// * `state` - Tauri-managed state containing the shared `FersContext`.
///
/// # Returns
///
/// * `Ok(())` if the scenario was successfully loaded.
/// * `Err(String)` containing an error message if loading failed (e.g., file not found,
///   invalid XML, or a Mutex lock error).
///
/// # Example (from frontend)
///
/// ```typescript
/// import { invoke } from '@tauri-apps/api/core';
/// await invoke('load_scenario_from_xml_file', { filepath: '/path/to/scenario.xml' });
/// ```
#[tauri::command]
fn load_scenario_from_xml_file(
    filepath: String,
    state: State<'_, FersState>,
) -> Result<(), String> {
    state
        .lock()
        .map_err(|e| e.to_string())?
        .load_scenario_from_xml_file(&filepath)
}

/// Retrieves the current in-memory scenario as a JSON string.
///
/// This command serializes the simulation state into JSON format, allowing the
/// frontend to display and edit the scenario. The JSON structure mirrors the
/// internal representation used by `libfers`.
///
/// # Parameters
///
/// * `state` - Tauri-managed state containing the shared `FersContext`.
///
/// # Returns
///
/// * `Ok(String)` containing the JSON representation of the scenario.
/// * `Err(String)` containing an error message if serialization failed or if the
///   Mutex could not be locked.
///
/// # Example (from frontend)
///
/// ```typescript
/// import { invoke } from '@tauri-apps/api/core';
/// const scenarioJson = await invoke<string>('get_scenario_as_json');
/// const scenario = JSON.parse(scenarioJson);
/// ```
#[tauri::command]
fn get_scenario_as_json(state: State<'_, FersState>) -> Result<String, String> {
    state
        .lock()
        .map_err(|e| e.to_string())?
        .get_scenario_as_json()
}

/// Retrieves the current in-memory scenario as a FERS XML string.
///
/// This command is typically used when the user wants to export the scenario
/// (potentially modified in the UI) back to the standard FERS XML format for
/// sharing or archival.
///
/// # Parameters
///
/// * `state` - Tauri-managed state containing the shared `FersContext`.
///
/// # Returns
///
/// * `Ok(String)` containing the XML representation of the scenario.
/// * `Err(String)` containing an error message if serialization failed or if the
///   Mutex could not be locked.
///
/// # Example (from frontend)
///
/// ```typescript
/// import { invoke } from '@tauri-apps/api/core';
/// const scenarioXml = await invoke<string>('get_scenario_as_xml');
/// // Save scenarioXml to a file using Tauri's fs plugin
/// ```
#[tauri::command]
fn get_scenario_as_xml(state: State<'_, FersState>) -> Result<String, String> {
    state
        .lock()
        .map_err(|e| e.to_string())?
        .get_scenario_as_xml()
}

/// Updates the in-memory scenario from a JSON string provided by the frontend.
///
/// This is the primary method for applying changes made in the UI back to the
/// simulation engine. The JSON is deserialized and used to rebuild the internal
/// C++ world representation.
///
/// # Parameters
///
/// * `json` - A JSON string representing the modified scenario structure.
/// * `state` - Tauri-managed state containing the shared `FersContext`.
///
/// # Returns
///
/// * `Ok(())` if the scenario was successfully updated.
/// * `Err(String)` containing an error message if deserialization failed, the JSON
///   structure was invalid, or the Mutex could not be locked.
///
/// # Example (from frontend)
///
/// ```typescript
/// import { invoke } from '@tauri-apps/api/core';
/// const updatedScenario = { /* modified scenario object */ };
/// await invoke('update_scenario_from_json', { json: JSON.stringify(updatedScenario) });
/// ```
#[tauri::command]
fn update_scenario_from_json(json: String, state: State<'_, FersState>) -> Result<(), String> {
    state
        .lock()
        .map_err(|e| e.to_string())?
        .update_scenario_from_json(&json)
}

/// Initializes and runs the Tauri application.
///
/// This function is the main entry point for the desktop application. It performs
/// the following setup steps:
///
/// 1. Creates a new `FersContext` by calling the FFI layer. If this fails, it
///    indicates a linking or initialization problem with `libfers`.
/// 2. Registers Tauri plugins for file dialogs, file system access, and shell operations.
/// 3. Stores the `FersContext` in Tauri's managed state, protected by a `Mutex`.
/// 4. Registers all Tauri commands so they can be invoked from the frontend.
/// 5. Launches the Tauri application event loop.
///
/// # Panics
///
/// This function will panic if:
/// * The `FersContext` cannot be created (indicating a problem with `libfers`).
/// * The Tauri application fails to start due to misconfiguration.
///
/// # Example
///
/// This function is typically called from `main.rs`:
///
/// ```rust
/// fn main() {
///     fers_ui_lib::run();
/// }
/// ```
#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    // Attempt to create the FFI context. This validates that libfers is correctly linked.
    let context = fers_api::FersContext::new()
        .expect("Failed to create FERS context. Is libfers linked correctly?");

    tauri::Builder::default()
        // Register Tauri plugins for UI functionality
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_opener::init())
        .plugin(tauri_plugin_fs::init())
        // Store the FersContext as managed state, accessible from all commands
        .manage(Mutex::new(context))
        // Register all Tauri commands that can be invoked from the frontend
        .invoke_handler(tauri::generate_handler![
            load_scenario_from_xml_file,
            get_scenario_as_json,
            get_scenario_as_xml,
            update_scenario_from_json
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}

#[cfg(test)]
mod tests {
    use super::fers_api;

    /// Verifies that the `libfers` C++ library is correctly linked.
    ///
    /// This test ensures that:
    /// 1. The Rust linker can find the `libfers.a` static library.
    /// 2. All required FFI symbols are present and callable.
    /// 3. The `FersContext::new()` wrapper successfully creates a C++ context.
    ///
    /// If this test fails:
    /// * Check that the `build.rs` script correctly specifies library search paths.
    /// * Verify that the C++ libraries have been built by CMake.
    /// * Ensure that `bindgen` generated the correct bindings from `api.h`.
    ///
    /// # Panics
    ///
    /// This test will panic if `FersContext::new()` returns `None`, indicating
    /// a failure to allocate or initialize the C++ context.
    #[test]
    fn it_links_libfers_and_creates_context() {
        // This test will fail at link time if the library is not found.
        // It will fail at runtime if the symbols don't match or creation fails.
        let _context = fers_api::FersContext::new().expect("FersContext::new() returned None");

        // The context is automatically destroyed when it goes out of scope due to `Drop`.
        // No explicit destroy call is needed.
    }
}
