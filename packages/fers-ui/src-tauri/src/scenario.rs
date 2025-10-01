// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

#![allow(non_snake_case)] // Allow camelCase field names to match JS

use serde::{Deserialize, Serialize};

// Helper for default values
fn default_as_true() -> bool { true }
fn default_as_pulsed() -> String { "pulsed".to_string() }
fn default_as_isotropic() -> String { "isotropic".to_string() }
fn default_as_constant() -> String { "constant".to_string() }
fn default_gp_id() -> String { "global-parameters".to_string() }
fn default_gp_type() -> String { "GlobalParameters".to_string() }
fn default_pulse_type_field() -> String { "Pulse".to_string() }
fn default_timing_type_field() -> String { "Timing".to_string() }
fn default_antenna_type_field() -> String { "Antenna".to_string() }
fn default_platform_type_field() -> String { "Platform".to_string() }

// --- TYPE DEFINITIONS (Mirroring scenarioStore.ts) ---
#[derive(Serialize, Deserialize, Debug, Default, Clone)]
pub struct ScenarioState {
    pub globalParameters: GlobalParameters,
    pub pulses: Vec<Pulse>,
    pub timings: Vec<Timing>,
    pub antennas: Vec<Antenna>,
    pub platforms: Vec<Platform>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct GlobalParameters {
    #[serde(default = "default_gp_id")]
    pub id: String,
    #[serde(rename = "type", default = "default_gp_type")]
    pub r#type: String, // 'type' is a keyword, so use r#
    pub simulation_name: String,
    pub start: f64,
    pub end: f64,
    pub rate: f64,
    pub simSamplingRate: Option<f64>,
    pub c: f64,
    pub random_seed: Option<f64>,
    pub adc_bits: i64,
    pub oversample_ratio: i64,
    pub export: ExportOptions,
}

impl Default for GlobalParameters {
    fn default() -> Self {
        Self {
            id: default_gp_id(),
            r#type: default_gp_type(),
            simulation_name: "FERS Simulation".to_string(),
            start: 0.0,
            end: 10.0,
            rate: 10000.0,
            simSamplingRate: None,
            c: 299792458.0,
            random_seed: None,
            adc_bits: 12,
            oversample_ratio: 1,
            export: ExportOptions::default(),
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Default, Clone)]
pub struct ExportOptions {
    pub xml: bool,
    pub csv: bool,
    #[serde(default = "default_as_true")]
    pub binary: bool,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Pulse {
    pub id: String,
    #[serde(rename = "type", default = "default_pulse_type_field")]
    pub r#type: String,
    pub name: String,
    pub pulseType: String, // "file" | "cw"
    pub power: f64,
    pub carrier: f64,
    pub filename: Option<String>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct NoiseEntry {
    pub id: String,
    pub alpha: f64,
    pub weight: f64,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Timing {
    pub id: String,
    #[serde(rename = "type", default = "default_timing_type_field")]
    pub r#type: String,
    pub name: String,
    pub frequency: f64,
    pub freqOffset: Option<f64>,
    pub randomFreqOffsetStdev: Option<f64>,
    pub phaseOffset: Option<f64>,
    pub randomPhaseOffsetStdev: Option<f64>,
    pub noiseEntries: Vec<NoiseEntry>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Antenna {
    pub id: String,
    #[serde(rename = "type", default = "default_antenna_type_field")]
    pub r#type: String,
    pub name: String,
    pub pattern: String, // "sinc" | "gaussian" etc.
    pub filename: Option<String>,
    pub efficiency: Option<f64>,
    pub alpha: Option<f64>,
    pub beta: Option<f64>,
    pub gamma: Option<f64>,
    pub azscale: Option<f64>,
    pub elscale: Option<f64>,
    pub diameter: Option<f64>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct PositionWaypoint {
    pub id: String,
    pub x: f64,
    pub y: f64,
    pub altitude: f64,
    pub time: f64,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct MotionPath {
    pub interpolation: String, // "static" | "linear" | "cubic"
    pub waypoints: Vec<PositionWaypoint>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(tag = "type", rename_all = "camelCase")]
pub enum Rotation {
    Fixed(FixedRotation),
    Path(RotationPath),
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct FixedRotation {
    pub startAzimuth: f64,
    pub startElevation: f64,
    pub azimuthRate: f64,
    pub elevationRate: f64,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct RotationWaypoint {
    pub id: String,
    pub azimuth: f64,
    pub elevation: f64,
    pub time: f64,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct RotationPath {
    pub interpolation: String,
    pub waypoints: Vec<RotationWaypoint>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(tag = "type", rename_all = "camelCase")]
pub enum PlatformComponent {
    None,
    Monostatic(Monostatic),
    Transmitter(Transmitter),
    Receiver(Receiver),
    Target(Target),
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Monostatic {
    pub name: String,
    #[serde(default = "default_as_pulsed")]
    pub radarType: String,
    pub window_skip: f64,
    pub window_length: f64,
    pub prf: f64,
    pub antennaId: Option<String>,
    pub pulseId: Option<String>,
    pub timingId: Option<String>,
    pub noiseTemperature: Option<f64>,
    pub noDirectPaths: bool,
    pub noPropagationLoss: bool,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Transmitter {
    pub name: String,
    #[serde(default = "default_as_pulsed")]
    pub radarType: String,
    pub prf: f64,
    pub antennaId: Option<String>,
    pub pulseId: Option<String>,
    pub timingId: Option<String>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Receiver {
    pub name: String,
    pub window_skip: f64,
    pub window_length: f64,
    pub prf: f64,
    pub antennaId: Option<String>,
    pub timingId: Option<String>,
    pub noiseTemperature: Option<f64>,
    pub noDirectPaths: bool,
    pub noPropagationLoss: bool,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Target {
    pub name: String,
    #[serde(default = "default_as_isotropic")]
    pub rcs_type: String,
    pub rcs_value: Option<f64>,
    pub rcs_filename: Option<String>,
    #[serde(default = "default_as_constant")]
    pub rcs_model: String,
    pub rcs_k: Option<f64>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Platform {
    pub id: String,
    #[serde(rename = "type", default = "default_platform_type_field")]
    pub r#type: String,
    pub name: String,
    pub motionPath: MotionPath,
    pub rotation: Rotation,
    pub component: PlatformComponent,
}
