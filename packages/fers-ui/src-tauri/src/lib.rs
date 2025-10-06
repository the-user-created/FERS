// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

mod scenario;
mod xml_handler;

// Learn more about Tauri commands at https://tauri.app/develop/calling-rust/
#[tauri::command]
fn greet(name: &str) -> String {
    format!("Hello, {}! You've been greeted from Rust!", name)
}

#[tauri::command]
fn generate_xml(scenario: scenario::ScenarioState) -> Result<String, String> {
    xml_handler::generate_xml_from_state(scenario)
}

#[tauri::command]
fn parse_xml(xml_content: String) -> Result<String, String> {
    // NOTE: The parser is a simplified stub.
    // It will not fully parse the example XML but demonstrates the round trip.
    xml_handler::parse_xml_to_state(xml_content)
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_opener::init())
        .plugin(tauri_plugin_fs::init())
        .invoke_handler(tauri::generate_handler![greet, generate_xml, parse_xml])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}

#[cfg(test)]
mod tests {
    /// Inner module to encapsulate the raw FFI bindings.
    mod ffi {
        #![allow(non_upper_case_globals)]
        #![allow(non_camel_case_types)]
        #![allow(non_snake_case)]
        #![allow(dead_code)]
        include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
    }

    /// A simple test to verify that the `libfers` static library is correctly linked.
    #[test]
    fn it_links_libfers_and_creates_context() {
        // This test will fail at link time if the library is not found.
        // It will fail at runtime if the symbols don't match.
        unsafe {
            // Call a simple function from the C++ API to verify linking.
            let context = ffi::fers_context_create();

            // Check if we got a valid pointer. This is a basic runtime check.
            assert!(!context.is_null(), "fers_context_create() returned null");

            // Call the corresponding destroy function to clean up.
            ffi::fers_context_destroy(context);
        }
    }
}
