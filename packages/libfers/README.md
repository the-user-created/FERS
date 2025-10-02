# libfers: The FERS Core Simulation Library

[![Build Status](https://github.com/the-user-created/FERS/actions/workflows/CMake.yml/badge.svg)](https://github.com/the-user-created/FERS/actions/workflows/CMake.yml)
[![License: GPL v2](https://img.shields.io/badge/License-GPLv2-blue.svg)](../../LICENSE)

**libfers** is the core C++ simulation engine for the **Flexible Extensible Radar Simulator (FERS)**. It contains all the logic for parsing scenarios, modeling physics, executing simulations, and processing output data.

This library is built with modern C++23 standards for optimal performance and maintainability. It exposes a stable C-style Foreign Function Interface (FFI) in `include/libfers/api.h`, allowing it to be used by various frontends, including the official `fers-cli` and `fers-ui` applications.

## Features

- **C-API:** A stable C-style API for creating and managing simulation contexts.
- **Signal-Level Modeling:** Creation of radar signal returns, including Doppler and phase modeling.
- **System Simulation:** Support for Monostatic, Multistatic, Continuous Wave (CW), and Pulsed radar systems.
- **Data Export:** Advanced data export in HDF5, CSV, and XML formats.
- **Geographic Visualization:** Generate KML files from scenarios.
- **Performance:** Efficient multithreading with a global thread pool.

## Dependencies

- **CMake** (3.13+)
- A C++23 compatible compiler (e.g., GCC 11+, Clang 14+)
- **libhdf5** (HDF5 support)
- **libxml2** (XML handling)
- **HighFive** & **GeographicLib** (included as git submodules)

## Building the Library and CLI

The build process will create both the `libfers` static library and the `fers-cli` executable.

### 1. Clone the Repository

Clone the repository and its submodules. All commands should be run from the root of the monorepo.

```bash
git clone --recursive https://github.com/the-user-created/FERS.git
cd FERS
```

If you've already cloned, ensure submodules are up to date:

```bash
git pull
git submodule update --init --recursive
```

### 2. Install System Dependencies

**On Ubuntu/Debian:**

```bash
sudo apt-get update && sudo apt-get install build-essential cmake libhdf5-dev libxml2-dev xxd
```

**On macOS (using Homebrew):**
A modern LLVM toolchain is required, as the default system Clang may be outdated.

```bash
brew install cmake hdf5 libxml2 llvm
```

### 3. Configure and Build

It is recommended to create a single build directory at the `packages` level.

**On Linux:**

```bash
# From the FERS/packages/ directory
mkdir build && cd build
cmake ..
make -j$(nproc)
```

> **Note on HDF5 on Ubuntu:** If you encounter linking issues with HDF5, you may need to specify paths manually:
> ```bash
> cmake .. -D FERS_LIB_HDF5="/usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5.so" \
> -D FERS_LIB_HDF5_HL="/usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5_hl.so" \
> -D CMAKE_CXX_FLAGS="-I/usr/include/hdf5/serial/"
> ```

**On macOS:**
You must point CMake to the Homebrew LLVM toolchain.

```bash
# From the FERS/packages/ directory
mkdir build && cd build
CC=/usr/local/opt/llvm/bin/clang CXX=/usr/local/opt/llvm/bin/clang++ cmake ..
make -j$(sysctl -n hw.ncpu)
```

The compiled `libfers.a` library will be located in `packages/build/libfers/` and the `fers-cli` executable will be in `packages/build/fers-cli/`.

> **Debug Build**: To create a debug build, add `-DCMAKE_BUILD_TYPE=Debug` to your `cmake` command. It's best practice
> to use separate build directories for release and debug versions.

### 4. Install (Optional)

You can install the library, headers, and CLI executable system-wide.

```bash
# From the build directory
sudo make install
sudo ldconfig # On Linux
```

## Regression Testing

A Python-based regression testing suite is available to validate simulation output.

To run the tests locally:

```bash
# Ensure you are in the packages/libfers directory
# Ensure a Release build exists at ../build

python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python3 run_sim_tests.py
```

The suite is integrated into our CI pipeline to ensure code changes do not introduce regressions.

## Documentation

Source code documentation can be generated using Doxygen.

```bash
# From the packages/libfers directory
doxygen Doxyfile
```

The output will be generated in the `docs/` directory.
