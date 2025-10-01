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
        .invoke_handler(tauri::generate_handler![
            greet,
            generate_xml,
            parse_xml
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
