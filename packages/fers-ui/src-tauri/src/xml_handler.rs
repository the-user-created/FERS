// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

#![allow(non_snake_case)]

use crate::scenario::*;
use quick_xml::de::from_str;
use quick_xml::events::{BytesEnd, BytesStart, Event};
use quick_xml::writer::Writer;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::io::Cursor;
use uuid::Uuid;

// Helper to map errors to a String for the Tauri result
fn map_err<E: std::fmt::Display>(e: E) -> String {
    e.to_string()
}

// ====================================================================================
//
//  XML GENERATION (SERIALIZATION) LOGIC
//
// ====================================================================================

fn write_simple_tag<W: std::io::Write>(
    writer: &mut Writer<W>,
    tag: &str,
    value: &str,
) -> Result<(), quick_xml::Error> {
    writer
        .create_element(tag)
        .write_text_content(quick_xml::events::BytesText::new(value))?;
    Ok(())
}

fn write_optional_tag<W: std::io::Write, T: ToString>(
    writer: &mut Writer<W>,
    tag: &str,
    value: &Option<T>,
) -> Result<(), quick_xml::Error> {
    if let Some(val) = value {
        write_simple_tag(writer, tag, &val.to_string())?;
    }
    Ok(())
}

pub fn generate_xml_from_state(scenario: ScenarioState) -> Result<String, String> {
    let mut writer = Writer::new_with_indent(Cursor::new(Vec::new()), b' ', 4);

    writer
        .write_event(Event::Decl(quick_xml::events::BytesDecl::new(
            "1.0", None, None,
        )))
        .map_err(map_err)?;
    writer
        .write_event(Event::DocType(quick_xml::events::BytesText::new(
            "simulation SYSTEM \"fers-xml.dtd\"",
        )))
        .map_err(map_err)?;

    let sim_tag = BytesStart::new("simulation")
        .with_attributes([("name", scenario.globalParameters.simulation_name.as_str())]);
    writer.write_event(Event::Start(sim_tag)).map_err(map_err)?;

    // <parameters>
    writer
        .write_event(Event::Start(BytesStart::new("parameters")))
        .map_err(map_err)?;
    let p = scenario.globalParameters;
    write_simple_tag(&mut writer, "starttime", &p.start.to_string()).map_err(map_err)?;
    write_simple_tag(&mut writer, "endtime", &p.end.to_string()).map_err(map_err)?;
    write_simple_tag(&mut writer, "rate", &p.rate.to_string()).map_err(map_err)?;
    write_simple_tag(&mut writer, "c", &p.c.to_string()).map_err(map_err)?;
    write_optional_tag(&mut writer, "simSamplingRate", &p.simSamplingRate).map_err(map_err)?;
    write_optional_tag(&mut writer, "randomseed", &p.random_seed.map(|s| s as u64))
        .map_err(map_err)?;
    write_simple_tag(&mut writer, "adc_bits", &p.adc_bits.to_string()).map_err(map_err)?;
    write_simple_tag(&mut writer, "oversample", &p.oversample_ratio.to_string())
        .map_err(map_err)?;
    let export_tag = BytesStart::new("export").with_attributes([
        ("binary", p.export.binary.to_string().as_str()),
        ("csv", p.export.csv.to_string().as_str()),
        ("xml", p.export.xml.to_string().as_str()),
    ]);
    writer
        .write_event(Event::Empty(export_tag))
        .map_err(map_err)?;
    writer
        .write_event(Event::End(BytesEnd::new("parameters")))
        .map_err(map_err)?;

    // Assets
    for pulse in &scenario.pulses {
        let mut tag = BytesStart::new("pulse");
        tag.push_attribute(("name", pulse.name.as_str()));
        let type_str = if pulse.pulseType == "cw" || pulse.pulseType == "continuous" {
            "continuous"
        } else {
            "file"
        };
        tag.push_attribute(("type", type_str));
        if let Some(filename) = &pulse.filename {
            tag.push_attribute(("filename", filename.as_str()));
        }
        writer.write_event(Event::Start(tag)).map_err(map_err)?;
        write_simple_tag(&mut writer, "power", &pulse.power.to_string()).map_err(map_err)?;
        write_simple_tag(&mut writer, "carrier", &pulse.carrier.to_string()).map_err(map_err)?;
        writer
            .write_event(Event::End(BytesEnd::new("pulse")))
            .map_err(map_err)?;
    }

    for timing in &scenario.timings {
        let tag = BytesStart::new("timing").with_attributes([("name", timing.name.as_str())]);
        writer.write_event(Event::Start(tag)).map_err(map_err)?;
        write_simple_tag(&mut writer, "frequency", &timing.frequency.to_string())
            .map_err(map_err)?;
        write_optional_tag(&mut writer, "freq_offset", &timing.freqOffset).map_err(map_err)?;
        write_optional_tag(
            &mut writer,
            "random_freq_offset_stdev",
            &timing.randomFreqOffsetStdev,
        )
        .map_err(map_err)?;
        write_optional_tag(&mut writer, "phase_offset", &timing.phaseOffset).map_err(map_err)?;
        write_optional_tag(
            &mut writer,
            "random_phase_offset_stdev",
            &timing.randomPhaseOffsetStdev,
        )
        .map_err(map_err)?;
        for entry in &timing.noiseEntries {
            writer
                .write_event(Event::Start(BytesStart::new("noise_entry")))
                .map_err(map_err)?;
            write_simple_tag(&mut writer, "alpha", &entry.alpha.to_string()).map_err(map_err)?;
            write_simple_tag(&mut writer, "weight", &entry.weight.to_string()).map_err(map_err)?;
            writer
                .write_event(Event::End(BytesEnd::new("noise_entry")))
                .map_err(map_err)?;
        }
        writer
            .write_event(Event::End(BytesEnd::new("timing")))
            .map_err(map_err)?;
    }

    for antenna in &scenario.antennas {
        let mut tag = BytesStart::new("antenna");
        tag.push_attribute(("name", antenna.name.as_str()));
        tag.push_attribute(("pattern", antenna.pattern.as_str()));
        if let Some(filename) = &antenna.filename {
            tag.push_attribute(("filename", filename.as_str()));
        }
        writer.write_event(Event::Start(tag)).map_err(map_err)?;
        write_optional_tag(&mut writer, "efficiency", &antenna.efficiency).map_err(map_err)?;
        match antenna.pattern.as_str() {
            "sinc" => {
                write_optional_tag(&mut writer, "alpha", &antenna.alpha).map_err(map_err)?;
                write_optional_tag(&mut writer, "beta", &antenna.beta).map_err(map_err)?;
                write_optional_tag(&mut writer, "gamma", &antenna.gamma).map_err(map_err)?;
            }
            "gaussian" => {
                write_optional_tag(&mut writer, "azscale", &antenna.azscale).map_err(map_err)?;
                write_optional_tag(&mut writer, "elscale", &antenna.elscale).map_err(map_err)?;
            }
            "squarehorn" | "parabolic" => {
                write_optional_tag(&mut writer, "diameter", &antenna.diameter).map_err(map_err)?;
            }
            _ => {}
        }
        writer
            .write_event(Event::End(BytesEnd::new("antenna")))
            .map_err(map_err)?;
    }

    let id_to_name: HashMap<_, _> = scenario
        .pulses
        .iter()
        .map(|p| (p.id.clone(), p.name.clone()))
        .chain(
            scenario
                .timings
                .iter()
                .map(|t| (t.id.clone(), t.name.clone())),
        )
        .chain(
            scenario
                .antennas
                .iter()
                .map(|a| (a.id.clone(), a.name.clone())),
        )
        .collect();

    for platform in &scenario.platforms {
        let platform_tag =
            BytesStart::new("platform").with_attributes([("name", platform.name.as_str())]);
        writer
            .write_event(Event::Start(platform_tag))
            .map_err(map_err)?;

        // Motion Path
        let mpath_tag = BytesStart::new("motionpath")
            .with_attributes([("interpolation", platform.motionPath.interpolation.as_str())]);
        writer
            .write_event(Event::Start(mpath_tag))
            .map_err(map_err)?;
        for wp in &platform.motionPath.waypoints {
            writer
                .write_event(Event::Start(BytesStart::new("positionwaypoint")))
                .map_err(map_err)?;
            write_simple_tag(&mut writer, "x", &wp.x.to_string()).map_err(map_err)?;
            write_simple_tag(&mut writer, "y", &wp.y.to_string()).map_err(map_err)?;
            write_simple_tag(&mut writer, "altitude", &wp.altitude.to_string()).map_err(map_err)?;
            write_simple_tag(&mut writer, "time", &wp.time.to_string()).map_err(map_err)?;
            writer
                .write_event(Event::End(BytesEnd::new("positionwaypoint")))
                .map_err(map_err)?;
        }
        writer
            .write_event(Event::End(BytesEnd::new("motionpath")))
            .map_err(map_err)?;

        // Rotation
        match &platform.rotation {
            Rotation::Fixed(r) => {
                writer
                    .write_event(Event::Start(BytesStart::new("fixedrotation")))
                    .map_err(map_err)?;
                write_simple_tag(&mut writer, "startazimuth", &r.startAzimuth.to_string())
                    .map_err(map_err)?;
                write_simple_tag(&mut writer, "startelevation", &r.startElevation.to_string())
                    .map_err(map_err)?;
                write_simple_tag(&mut writer, "azimuthrate", &r.azimuthRate.to_string())
                    .map_err(map_err)?;
                write_simple_tag(&mut writer, "elevationrate", &r.elevationRate.to_string())
                    .map_err(map_err)?;
                writer
                    .write_event(Event::End(BytesEnd::new("fixedrotation")))
                    .map_err(map_err)?;
            }
            Rotation::Path(r) => {
                let rpath_tag = BytesStart::new("rotationpath")
                    .with_attributes([("interpolation", r.interpolation.as_str())]);
                writer
                    .write_event(Event::Start(rpath_tag))
                    .map_err(map_err)?;
                for wp in &r.waypoints {
                    writer
                        .write_event(Event::Start(BytesStart::new("rotationwaypoint")))
                        .map_err(map_err)?;
                    write_simple_tag(&mut writer, "azimuth", &wp.azimuth.to_string())
                        .map_err(map_err)?;
                    write_simple_tag(&mut writer, "elevation", &wp.elevation.to_string())
                        .map_err(map_err)?;
                    write_simple_tag(&mut writer, "time", &wp.time.to_string()).map_err(map_err)?;
                    writer
                        .write_event(Event::End(BytesEnd::new("rotationwaypoint")))
                        .map_err(map_err)?;
                }
                writer
                    .write_event(Event::End(BytesEnd::new("rotationpath")))
                    .map_err(map_err)?;
            }
        }

        // Component
        let get_name = |id: &Option<String>| {
            id.as_ref()
                .and_then(|i| id_to_name.get(i).map(|s| s.as_str()))
        };

        match &platform.component {
            PlatformComponent::Receiver(r) => {
                let mut tag = BytesStart::new("receiver");
                tag.push_attribute(("name", r.name.as_str()));
                if let Some(name) = get_name(&r.antennaId) {
                    tag.push_attribute(("antenna", name));
                }
                if let Some(name) = get_name(&r.timingId) {
                    tag.push_attribute(("timing", name));
                }
                writer.write_event(Event::Start(tag)).map_err(map_err)?;
                write_simple_tag(&mut writer, "window_skip", &r.window_skip.to_string())
                    .map_err(map_err)?;
                write_simple_tag(&mut writer, "window_length", &r.window_length.to_string())
                    .map_err(map_err)?;
                write_simple_tag(&mut writer, "prf", &r.prf.to_string()).map_err(map_err)?;
                write_optional_tag(&mut writer, "noise_temp", &r.noiseTemperature)
                    .map_err(map_err)?;
                writer
                    .write_event(Event::End(BytesEnd::new("receiver")))
                    .map_err(map_err)?;
            }
            PlatformComponent::Transmitter(t) => {
                let mut tag = BytesStart::new("transmitter");
                tag.push_attribute(("name", t.name.as_str()));
                tag.push_attribute(("type", "continuous")); // Simplified for example
                if let Some(name) = get_name(&t.antennaId) {
                    tag.push_attribute(("antenna", name));
                }
                if let Some(name) = get_name(&t.pulseId) {
                    tag.push_attribute(("pulse", name));
                }
                if let Some(name) = get_name(&t.timingId) {
                    tag.push_attribute(("timing", name));
                }
                writer.write_event(Event::Start(tag)).map_err(map_err)?;
                write_simple_tag(&mut writer, "prf", &t.prf.to_string()).map_err(map_err)?;
                writer
                    .write_event(Event::End(BytesEnd::new("transmitter")))
                    .map_err(map_err)?;
            }
            PlatformComponent::Target(t) => {
                let target_tag =
                    BytesStart::new("target").with_attributes([("name", t.name.as_str())]);
                writer
                    .write_event(Event::Start(target_tag))
                    .map_err(map_err)?;
                let mut rcs_tag = BytesStart::new("rcs");
                rcs_tag.push_attribute(("type", t.rcs_type.as_str()));
                if let Some(f) = &t.rcs_filename {
                    rcs_tag.push_attribute(("filename", f.as_str()));
                }
                writer.write_event(Event::Start(rcs_tag)).map_err(map_err)?;
                write_optional_tag(&mut writer, "value", &t.rcs_value).map_err(map_err)?;
                writer
                    .write_event(Event::End(BytesEnd::new("rcs")))
                    .map_err(map_err)?;
                if t.rcs_model != "constant" {
                    let model_tag =
                        BytesStart::new("model").with_attributes([("type", t.rcs_model.as_str())]);
                    writer
                        .write_event(Event::Start(model_tag))
                        .map_err(map_err)?;
                    write_optional_tag(&mut writer, "k", &t.rcs_k).map_err(map_err)?;
                    writer
                        .write_event(Event::End(BytesEnd::new("model")))
                        .map_err(map_err)?;
                }
                writer
                    .write_event(Event::End(BytesEnd::new("target")))
                    .map_err(map_err)?;
            }
            _ => {} // Other components not implemented for brevity
        }

        writer
            .write_event(Event::End(BytesEnd::new("platform")))
            .map_err(map_err)?;
    }

    writer
        .write_event(Event::End(BytesEnd::new("simulation")))
        .map_err(map_err)?;

    let result = writer.into_inner().into_inner();
    String::from_utf8(result).map_err(map_err)
}

// ====================================================================================
//
//  XML PARSING (DESERIALIZATION) LOGIC
//
// ====================================================================================

// --- XML-Direct Mapping Structs ---
// These structs are designed to be deserialized directly from the XML structure.
#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlSimulation {
    #[serde(rename = "@name")]
    name: String,
    parameters: XmlParameters,
    #[serde(rename = "pulse", default)]
    pulses: Vec<XmlPulse>,
    #[serde(rename = "timing", default)]
    timings: Vec<XmlTiming>,
    #[serde(rename = "antenna", default)]
    antennas: Vec<XmlAntenna>,
    #[serde(rename = "platform", default)]
    platforms: Vec<XmlPlatform>,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlParameters {
    starttime: f64,
    endtime: f64,
    rate: f64,
    c: f64,
    simSamplingRate: Option<f64>,
    randomseed: Option<u64>,
    adc_bits: i64,
    oversample: i64,
    export: XmlExport,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlExport {
    #[serde(rename = "@binary")]
    binary: bool,
    #[serde(rename = "@csv")]
    csv: bool,
    #[serde(rename = "@xml")]
    xml: bool,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlPulse {
    #[serde(rename = "@name")]
    name: String,
    #[serde(rename = "@type")]
    ptype: String,
    #[serde(rename = "@filename")]
    filename: Option<String>,
    power: f64,
    carrier: f64,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlTiming {
    #[serde(rename = "@name")]
    name: String,
    frequency: f64,
    freq_offset: Option<f64>,
    random_freq_offset_stdev: Option<f64>,
    phase_offset: Option<f64>,
    random_phase_offset_stdev: Option<f64>,
    #[serde(rename = "noise_entry", default)]
    noise_entries: Vec<XmlNoiseEntry>,
}

#[derive(Debug, Serialize, Deserialize, PartialEq, Clone)]
struct XmlNoiseEntry {
    alpha: f64,
    weight: f64,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlAntenna {
    #[serde(rename = "@name")]
    name: String,
    #[serde(rename = "@pattern")]
    pattern: String,
    #[serde(rename = "@filename")]
    filename: Option<String>,
    efficiency: Option<f64>,
    alpha: Option<f64>,
    beta: Option<f64>,
    gamma: Option<f64>,
    azscale: Option<f64>,
    elscale: Option<f64>,
    diameter: Option<f64>,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlPlatform {
    #[serde(rename = "@name")]
    name: String,
    motionpath: XmlMotionPath,
    fixedrotation: Option<XmlFixedRotation>,
    // In a real scenario, you'd handle <rotationpath> as well, likely with an enum
    transmitter: Option<XmlTransmitter>,
    receiver: Option<XmlReceiver>,
    target: Option<XmlTarget>,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlMotionPath {
    #[serde(rename = "@interpolation")]
    interpolation: String,
    #[serde(rename = "positionwaypoint", default)]
    waypoints: Vec<XmlPositionWaypoint>,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlPositionWaypoint {
    x: f64,
    y: f64,
    altitude: f64,
    time: f64,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlFixedRotation {
    startazimuth: f64,
    startelevation: f64,
    azimuthrate: f64,
    elevationrate: f64,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlTransmitter {
    #[serde(rename = "@name")]
    name: String,
    #[serde(rename = "@antenna")]
    antenna: String,
    #[serde(rename = "@pulse")]
    pulse: String,
    #[serde(rename = "@timing")]
    timing: String,
    prf: f64,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlReceiver {
    #[serde(rename = "@name")]
    name: String,
    #[serde(rename = "@antenna")]
    antenna: String,
    #[serde(rename = "@timing")]
    timing: String,
    window_skip: f64,
    window_length: f64,
    prf: f64,
    noise_temp: Option<f64>,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlTarget {
    #[serde(rename = "@name")]
    name: String,
    rcs: XmlRcs,
    model: Option<XmlModel>,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlRcs {
    #[serde(rename = "@type")]
    rtype: String,
    #[serde(rename = "@filename")]
    filename: Option<String>,
    value: Option<f64>,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct XmlModel {
    #[serde(rename = "@type")]
    mtype: String,
    k: Option<f64>,
}

// --- Transformer Function ---
// Converts the XML-mapped structs into the UI's ScenarioState format
fn transform_xml_to_state(xml: XmlSimulation) -> ScenarioState {
    let mut name_to_id = HashMap::new();

    let pulses: Vec<Pulse> = xml
        .pulses
        .into_iter()
        .map(|p| {
            let id = Uuid::new_v4().to_string();
            name_to_id.insert(p.name.clone(), id.clone());
            Pulse {
                id,
                r#type: "Pulse".to_string(),
                name: p.name,
                pulseType: if p.ptype == "continuous" {
                    "cw".to_string()
                } else {
                    p.ptype
                },
                power: p.power,
                carrier: p.carrier,
                filename: p.filename,
            }
        })
        .collect();

    let timings: Vec<Timing> = xml
        .timings
        .into_iter()
        .map(|t| {
            let id = Uuid::new_v4().to_string();
            name_to_id.insert(t.name.clone(), id.clone());
            Timing {
                id,
                r#type: "Timing".to_string(),
                name: t.name,
                frequency: t.frequency,
                freqOffset: t.freq_offset,
                randomFreqOffsetStdev: t.random_freq_offset_stdev,
                phaseOffset: t.phase_offset,
                randomPhaseOffsetStdev: t.random_phase_offset_stdev,
                noiseEntries: t
                    .noise_entries
                    .into_iter()
                    .map(|ne| NoiseEntry {
                        id: Uuid::new_v4().to_string(),
                        alpha: ne.alpha,
                        weight: ne.weight,
                    })
                    .collect(),
            }
        })
        .collect();

    let antennas: Vec<Antenna> = xml
        .antennas
        .into_iter()
        .map(|a| {
            let id = Uuid::new_v4().to_string();
            name_to_id.insert(a.name.clone(), id.clone());
            Antenna {
                id,
                r#type: "Antenna".to_string(),
                name: a.name,
                pattern: a.pattern,
                filename: a.filename,
                efficiency: a.efficiency,
                alpha: a.alpha,
                beta: a.beta,
                gamma: a.gamma,
                azscale: a.azscale,
                elscale: a.elscale,
                diameter: a.diameter,
            }
        })
        .collect();

    let platforms: Vec<Platform> = xml
        .platforms
        .into_iter()
        .map(|p| {
            let component = if let Some(t) = p.transmitter {
                PlatformComponent::Transmitter(Transmitter {
                    name: t.name,
                    radarType: "pulsed".to_string(),
                    prf: t.prf,
                    antennaId: name_to_id.get(&t.antenna).cloned(),
                    pulseId: name_to_id.get(&t.pulse).cloned(),
                    timingId: name_to_id.get(&t.timing).cloned(),
                })
            } else if let Some(r) = p.receiver {
                PlatformComponent::Receiver(Receiver {
                    name: r.name,
                    window_skip: r.window_skip,
                    window_length: r.window_length,
                    prf: r.prf,
                    antennaId: name_to_id.get(&r.antenna).cloned(),
                    timingId: name_to_id.get(&r.timing).cloned(),
                    noiseTemperature: r.noise_temp,
                    noDirectPaths: false, // These attributes aren't in the example
                    noPropagationLoss: false,
                })
            } else if let Some(t) = p.target {
                PlatformComponent::Target(Target {
                    name: t.name,
                    rcs_type: t.rcs.rtype,
                    rcs_value: t.rcs.value,
                    rcs_filename: t.rcs.filename,
                    rcs_model: t
                        .model
                        .as_ref()
                        .map_or("constant".to_string(), |m| m.mtype.clone()),
                    rcs_k: t.model.and_then(|m| m.k),
                })
            } else {
                PlatformComponent::None
            };

            Platform {
                id: Uuid::new_v4().to_string(),
                r#type: "Platform".to_string(),
                name: p.name,
                motionPath: MotionPath {
                    interpolation: p.motionpath.interpolation,
                    waypoints: p
                        .motionpath
                        .waypoints
                        .into_iter()
                        .map(|wp| PositionWaypoint {
                            id: Uuid::new_v4().to_string(),
                            x: wp.x,
                            y: wp.y,
                            altitude: wp.altitude,
                            time: wp.time,
                        })
                        .collect(),
                },
                rotation: p.fixedrotation.map_or_else(
                    || {
                        Rotation::Fixed(FixedRotation {
                            startAzimuth: 0.0,
                            startElevation: 0.0,
                            azimuthRate: 0.0,
                            elevationRate: 0.0,
                        })
                    },
                    |r| {
                        Rotation::Fixed(FixedRotation {
                            startAzimuth: r.startazimuth,
                            startElevation: r.startelevation,
                            azimuthRate: r.azimuthrate,
                            elevationRate: r.elevationrate,
                        })
                    },
                ),
                component,
            }
        })
        .collect();

    ScenarioState {
        globalParameters: GlobalParameters {
            id: "global-parameters".to_string(),
            r#type: "GlobalParameters".to_string(),
            simulation_name: xml.name,
            start: xml.parameters.starttime,
            end: xml.parameters.endtime,
            rate: xml.parameters.rate,
            simSamplingRate: xml.parameters.simSamplingRate,
            c: xml.parameters.c,
            random_seed: xml.parameters.randomseed.map(|s| s as f64),
            adc_bits: xml.parameters.adc_bits,
            oversample_ratio: xml.parameters.oversample,
            export: ExportOptions {
                xml: xml.parameters.export.xml,
                csv: xml.parameters.export.csv,
                binary: xml.parameters.export.binary,
            },
        },
        pulses,
        timings,
        antennas,
        platforms,
    }
}

pub fn parse_xml_to_state(xml_content: String) -> Result<String, String> {
    let parsed_xml: XmlSimulation = from_str(&xml_content).map_err(map_err)?;
    let state = transform_xml_to_state(parsed_xml);
    serde_json::to_string(&state).map_err(map_err)
}
