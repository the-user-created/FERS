// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

use std::env;
use std::path::PathBuf;

fn main() {
    // --- 1. Link C++ libraries ---

    // -- Link libraries built within the project --
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let build_dir = manifest_dir.join("../../../build");

    let libfers_lib_dir = build_dir.join("packages/libfers");
    let geographiclib_lib_dir = build_dir.join("packages/libfers/third_party/geographiclib/src");

    println!(
        "cargo:rustc-link-search=native={}",
        libfers_lib_dir.display()
    );
    println!(
        "cargo:rustc-link-search=native={}",
        geographiclib_lib_dir.display()
    );

    println!("cargo:rustc-link-lib=static=fers");
    println!("cargo:rustc-link-lib=dylib=GeographicLib");

    // -- Find and link system dependencies using pkg-config --
    pkg_config::probe_library("libxml-2.0").unwrap();
    pkg_config::probe_library("hdf5").unwrap();

    // Link correct C++ standard library
    if cfg!(target_os = "macos") {
        println!("cargo:rustc-link-lib=c++");
    } else if cfg!(target_os = "linux") {
        println!("cargo:rustc-link-lib=stdc++");
    }
    // On Windows with MSVC, the C++ standard library is linked automatically.

    // --- 2. Generate Rust bindings for the C++ API ---
    let header_path = manifest_dir.join("../../libfers/include/libfers/api.h");
    println!("cargo:rerun-if-changed={}", header_path.display());

    let bindings = bindgen::Builder::default()
        .header(header_path.to_str().unwrap())
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .allowlist_function("fers_.*")
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");

    // --- 3. Tauri build step ---
    tauri_build::build();
}
