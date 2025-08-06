# FERS—The Flexible Extensible Radar Simulator

[![Build Status](https://github.com/the-user-created/FERS/actions/workflows/CMake.yml/badge.svg)](https://github.com/the-user-created/FERS/actions/workflows/CMake.yml)
[![GitHub issues](https://img.shields.io/github/issues/the-user-created/FERS.svg)](https://github.com/the-user-created/FERS/issues)
[![License](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://opensource.org/licenses/GPL-3.0)

## Overview

FERS (Flexible Extensible Radar Simulator) is a C++ radar simulation tool that models various radar systems, both
traditional and modern.
This simulator allows detailed signal-level simulations and performance assessments of radar systems under different
conditions and configurations.

The FERS software has been **modernized** to leverage modern C++ standards (C++20/23) for improved performance,
maintainability, and modularity.
Key enhancements include optimized multithreading, better memory management, and updated code features.

### Key Features

- Creation of radar signal returns, including Doppler and phase modeling
- Simulation of Multistatic and Monostatic radar systems
- Support for both CW (Continuous Wave) and Pulsed radars
- Multipath propagation effects
- Data export in CSV, XML, and HDF5 formats
- Geographic scenario visualization via KML export
- Enhanced memory management using smart pointers
- Multithreading with a global thread pool

### Authors

- Marc Brooker (Original Author)
- Michael Inggs (Original Author)
- David Young (Modernization and Refactor)

## Dependencies

FERS relies on the following libraries:

- **HighFive** (included as a git submodule)
- **GeographicLib** (included as a git submodule)
- **libhdf5** (HDF5 support)
- **libxml2** (XML handling)
- **python3.11** (for additional scripting capabilities)

> **Note**: The **Boost** library has been **completely removed** in this modernization, and **FFTW3** is no longer
> required.

## Installation Instructions

### Local Installation on Ubuntu/Linux

You can also run FERS on **Windows 10** using **Windows Subsystem for Linux (WSL)**.
However, installing FERS on Windows 10+ has not been tested, and therefore is not officially supported.
For WSL setup instructions, refer to the
appropriate [WSL setup guide](https://learn.microsoft.com/en-us/windows/wsl/install) (link to WSL setup).

Before beginning, update your system’s package list and upgrade installed packages:

```bash
sudo apt-get update && sudo apt-get upgrade
```

#### Step 1: Install Dependencies

Install the required libraries:

```bash
sudo apt-get install libhdf5-dev libhdf5-serial-dev libxml2-dev build-essential cmake python3.11 python3.11-venv
sudo apt-get install cmake-qt-gui # Optional: CMake GUI
```

> **Note**: If you encounter issues while trying to install python3.11, you may need to use the deadsnakes PPA:
> ```bash
> sudo add-apt-repository ppa:deadsnakes/ppa
> sudo apt-get update
> sudo apt-get install python3.11 python3.11-venv
> ```
> Then ensure to use -D PYTHON_EXECUTABLE=/usr/bin/python3.11 when running CMake.

#### Step 2: Clone the Repository

Clone the FERS repository and initialize the submodules:

```bash
git clone --recursive https://github.com/the-user-created/FERS.git
cd FERS
```

> **Note:** If you have already cloned the repository, you can update to the latest version and sync the submodules by
> running `git pull` followed by `git submodule update --init --recursive` from the project's root directory.

#### Step 3: Build FERS

Create a build directory, configure, and compile the project:

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The compiled `fers` binary will be placed in the `build/src/` directory.

> **Note**: To build in Debug mode, replace `cmake ..` with:
> ```bash
> cmake -DCMAKE_BUILD_TYPE=Debug ..
> ```
> It’s recommended to use separate build directories for release and debug builds.

#### Step 4: Install FERS

Optionally, you can install FERS system-wide:

```bash
sudo make install
sudo ldconfig
```

### Advanced Installation Notes for Ubuntu 15+

Ubuntu versions 15+ may have issues with the HDF5 libraries. In such cases, manually link the libraries:

```bash
cmake -D FERS_LIB_HDF5="/usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5.so" \
      -D FERS_LIB_HDF5_HL="/usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5_hl.so" \
      -D CMAKE_CXX_FLAGS="-I/usr/include/hdf5/serial/" ../
make -j$(nproc)
sudo make install
sudo ldconfig
```

You can use **CMake GUI** for manual configuration by navigating to the HDF5 paths as needed.

## Regression Testing Suite

The FERS modernization includes a comprehensive **regression testing suite**
to ensure the accuracy and reliability of the simulation results after updates or changes to the codebase.
This suite is located in the `test/sim_tests/` directory and contains multiple test cases,
each stored in its own folder (e.g., `test1/`, `test2/`, etc.).

### How the Regression Suite Works:

- Each test case folder contains:
    - The `.fersxml` and waveform file (`.csv` or `.h5`).
    - Any additional files required to run the simulation.
    - An `expected_output` folder, which holds the expected results for each test case.
- To run the regression suite, first build the Release version of FERS in a directory named `build/` and then use the
  `run_sim_tests.py` script, which automatically executes every test case and compares the output to the expected
  results.
- **CI Integration**: The regression suite is integrated with the Continuous Integration (CI) build.
  The `run_sim_tests.py` script is invoked as part of the CI process,
  and if any test case fails, the build will fail, ensuring that code changes do not introduce errors.

To run the regression tests locally:

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python3 run_sim_tests.py
```

The script will report on the overall result of the test suite, including any test cases that fail.

> **Note**: If any major changes are made to the codebase which are intended to change the output of the simulation,
> the expected results in the `expected_output` folders should be updated accordingly otherwise the regression tests
> will fail.

## Scenario Visualization (KML Export)

FERS can generate KML (Keyhole Markup Language) files to create a geographic visualization of your simulation scenario,
which can be viewed in applications like Google Earth. This feature is fully integrated into the FERS executable and
provides a geodetically accurate representation of platforms, paths, and antenna patterns.

This integrated visualizer replaces the legacy standalone `kml_visualiser` tool and builds upon the original concept by
Michael Altshuler. It uses the **GeographicLib** library to ensure that the local simulation coordinates are accurately
projected onto the WGS84 ellipsoid.

#### How it Works

The KML generator interprets the FERS local Cartesian coordinate system as an East-North-Up (ENU) tangent plane. You can
define the geographic anchor point for this plane (i.e., the location of `(0,0,0)`) in your scenario file.

To define the origin, add an `<origin>` tag to the `<parameters>` section of your `.fersxml` file. The altitude is
specified in meters above Mean Sea Level (MSL).

**XML Configuration Example:**

```xml

<parameters>
    ...
    <!-- Set the simulation's (0,0,0) to a specific point on Earth -->
    <origin latitude="34.0522" longitude="-118.2437" altitude="71.0"/>
    ...
</parameters>
```

> **Note**: If the `<origin>` tag is omitted, the location defaults to the University of Cape Town, South Africa.

#### Generating a KML File

To generate the KML file, run FERS with the `--kml` command-line argument, specifying the path to your scenario and the
desired output file. FERS will parse the scenario, generate the KML, and exit without running the full simulation.

**Command-line Example:**

```bash
# From the build directory
./src/fers --kml=path/to/my_scenario.kml path/to/your_scenario.fersxml
```

You can then open `my_scenario.kml` in any KML-compatible viewer.

## Documentation

For detailed documentation on FERS, refer to the `docs/` directory in the repository.

### Doxygen Documentation

The Doxygen documentation for FERS can be generated using the following steps:

```bash
cd FERS # Navigate to the root directory of the FERS repository
doxygen Doxyfile
```

## Contributing

If you would like to contribute to FERS, please refer to the [CONTRIBUTING.md](CONTRIBUTING.md) file for guidelines.

## License

FERS is licensed under the GNU General Public License (GPL) v3. See the [LICENSE](LICENSE) file for more details.
