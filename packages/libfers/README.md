# FERS Core Simulator

[![Build Status](https://github.com/the-user-created/FERS/actions/workflows/CMake.yml/badge.svg)](https://github.com/the-user-created/FERS/actions/workflows/CMake.yml)
[![License: GPL v2](https://img.shields.io/badge/License-GPLv2-blue.svg)](../../LICENSE)

**FERS (Flexible Extensible Radar Simulator)** is a high-performance, command-line C++ tool that models various radar
systems at the signal level. It allows for detailed simulations and performance assessments of radar systems under
different conditions and configurations.

This engine is built with modern C++23 standards for optimal performance, maintainability, and modularity, featuring
optimized multithreading, robust memory management, and a powerful feature set.

## Features

- Creation of radar signal returns, including Doppler and phase modeling.
- Simulation of Multistatic and Monostatic radar systems.
- Support for both Continuous Wave (CW) and Pulsed radars.
- Data export in HDF5, CSV, and XML formats.
- Geographic scenario visualization via KML export.
- Efficient multithreading with a global thread pool.

## Dependencies

- **CMake** (3.13+)
- A C++23 compatible compiler (e.g., GCC 11+, Clang 14+)
- **libhdf5** (HDF5 support)
- **libxml2** (XML handling)
- **HighFive** & **GeographicLib** (included as git submodules)

## Building from Source

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
sudo apt-get update && sudo apt-get upgrade
sudo apt-get install build-essential cmake libhdf5-dev libxml2-dev
```

**On macOS (using Homebrew):**
A modern LLVM toolchain is required, as the default system Clang may be outdated.

```bash
brew install cmake hdf5 libxml2 llvm
```

### 3. Configure and Build

It is recommended to create a separate build directory.

**On Linux:**

```bash
# From the packages/fers directory
mkdir build && cd build
cmake ..
make -j$(nproc)
```

> NOTE: Ubuntu versions 15+ may have issues with the HDF5 libraries. In such cases, manually link the libraries:
> ```bash
> cmake -D FERS_LIB_HDF5="/usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5.so" \
> -D FERS_LIB_HDF5_HL="/usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5_hl.so" \
> -D CMAKE_CXX_FLAGS="-I/usr/include/hdf5/serial/" ../
> ```

**On macOS:**
You must point CMake to the Homebrew LLVM toolchain.

```bash
# From the packages/fers directory
mkdir build && cd build
CC=/usr/local/opt/llvm/bin/clang CXX=/usr/local/opt/llvm/bin/clang++ cmake ..
make -j$(sysctl -n hw.ncpu)
```

The compiled `fers` binary will be located in the `build/src/` directory.

> **Debug Build**: To create a debug build, add `-DCMAKE_BUILD_TYPE=Debug` to your `cmake` command. It's best practice
> to use separate build directories for release and debug versions.

### 4. Install (Optional)

You can install the `fers` binary system-wide.

```bash
# From the build directory
sudo make install
sudo ldconfig # On Linux
```

## Usage

Run the simulator from the command line, providing the path to a scenario XML file.

```bash
# From the packages/fers/build directory
./src/fers path/to/your/scenario.fersxml
```

## Regression Testing

A Python-based regression testing suite is available in the `test/sim_tests/` directory to validate simulation output.

To run the tests locally:

```bash
# Ensure you are in the packages/fers directory
# Ensure a Release build exists at ./build

python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python3 test/sim_tests/run_sim_tests.py
```

The suite is integrated into our CI pipeline to ensure code changes do not introduce regressions.

## Documentation

Source code documentation can be generated using Doxygen.

```bash
# From the packages/fers directory
doxygen Doxyfile
```

The output will be generated in the `docs/` directory.
