// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

mod fers_api;

use std::sync::Mutex;
use tauri::State;

// Define a type alias for the managed state for convenience.
// This holds the singleton FersContext, protected by a Mutex for thread-safe access.
type FersState = Mutex<fers_api::FersContext>;

// --- Tauri Commands ---

/// Loads a scenario from an XML file, replacing the current in-memory scenario.
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
#[tauri::command]
fn get_scenario_as_json(state: State<'_, FersState>) -> Result<String, String> {
    state
        .lock()
        .map_err(|e| e.to_string())?
        .get_scenario_as_json()
}

/// Retrieves the current in-memory scenario as an XML string for export.
#[tauri::command]
fn get_scenario_as_xml(state: State<'_, FersState>) -> Result<String, String> {
    state
        .lock()
        .map_err(|e| e.to_string())?
        .get_scenario_as_xml()
}

/// Updates the in-memory scenario from a JSON string provided by the UI.
#[tauri::command]
fn update_scenario_from_json(json: String, state: State<'_, FersState>) -> Result<(), String> {
    state
        .lock()
        .map_err(|e| e.to_string())?
        .update_scenario_from_json(&json)
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    let context = fers_api::FersContext::new()
        .expect("Failed to create FERS context. Is libfers linked correctly?");

    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_opener::init())
        .plugin(tauri_plugin_fs::init())
        .manage(Mutex::new(context)) // Add the FersContext to Tauri's managed state
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

    /// A simple test to verify that the `libfers` static library is correctly linked
    /// and that the safe wrapper works.
    #[test]
    fn it_links_libfers_and_creates_context() {
        // This test will fail at link time if the library is not found.
        // It will fail at runtime if the symbols don't match or creation fails.
        let _context = fers_api::FersContext::new().expect("FersContext::new() returned None");

        // The context is automatically destroyed when it goes out of scope due to `Drop`.
        // No explicit destroy call is needed.
    }
}
