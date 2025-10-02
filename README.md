# FERS: The Flexible Extensible Radar Simulator

<!-- TODO: Build status should be present for both ui and core -->
[![FERS Core CI](https://github.com/the-user-created/FERS/actions/workflows/CMake.yml/badge.svg)](https://github.com/the-user-created/FERS/actions/workflows/CMake.yml)
[![Documentation](https://github.com/the-user-created/FERS/actions/workflows/docs.yml/badge.svg)](https://github.com/the-user-created/FERS/actions/workflows/docs.yml)
[![GitHub issues](https://img.shields.io/github/issues/the-user-created/FERS.svg)](https://github.com/the-user-created/FERS/issues)
[![License: GPL v2](https://img.shields.io/badge/License-GPLv2-blue.svg)](https://github.com/the-user-created/FERS/blob/master/LICENSE)

![Tech Stack](https://img.shields.io/badge/Core-C%2B%2B%2023-00599C?logo=cplusplus)
![Tech Stack](https://img.shields.io/badge/UI-Tauri%20%7C%20React-20232A?logo=react)

FERS is a comprehensive suite of tools for signal-level radar simulation. It consists of a high-performance C++
simulation engine and a modern, intuitive graphical user interface for building and visualizing complex scenarios.

This repository is structured as a monorepo, containing the core simulator, the UI, and the shared data schema as
semi-independent packages.

---

**[View the Live Documentation](https://the-user-created.github.io/FERS/)**

---

## Key Features

- **High-Performance Core:** A modernized C++23 engine capable of detailed signal-level simulation with optimized
  multithreading.
- **Visual Scenario Builder:** An intuitive 3D interface to construct, configure, and visualize radar scenarios.
- **Flexible System Modeling:** Simulate a wide range of radar systems, including monostatic, multistatic, pulsed, and
  continuous wave (CW).
- **Advanced Data Export:** Output simulation data in HDF5, CSV, and XML formats for analysis.
- **Geographic Visualization:** Generate KML files from scenarios for accurate visualization in tools like Google Earth.
- **Modern Documentation:** A continuously updated and deployed [documentation site](https://the-user-created.github.io/FERS/) 
  with a searchable interface, generated directly from the source code.
- **Unified Schema:** A central XML schema ensures consistency and serves as the single source of truth for scenarios
  across the simulator and the UI.

## Repository Structure

This monorepo contains the following packages:

- `packages/libfers`: The core C++ radar simulation library. It contains all the core logic, physics, and file processing capabilities, exposed through a C-style API.
- `packages/fers-cli`: A lightweight command-line interface that acts as a wrapper around `libfers`, providing backward compatibility with the original FERS executable.
- `packages/fers-ui`: A modern desktop application built with Tauri and React. It provides a graphical interface for
  creating, editing, and visualizing FERS scenarios by linking against `libfers`.
- `packages/schema`: The XML Schema Definition (XSD) and Document Type Definition (DTD) that define the structure of
  FERS scenario files. This schema is the contract between the UI and the core simulator.

## Getting Started

To get started with a specific package, please refer to its dedicated README file for detailed instructions on
dependencies, building, and usage.

- **For the core C++ library, see:** [`packages/libfers/README.md`](packages/libfers/README.md)
- **For the command-line interface, see:** [`packages/fers-cli/README.md`](packages/fers-cli/README.md)
- **For the graphical user interface, see:** [`packages/fers-ui/README.md`](packages/fers-ui/README.md)

## Contributing

We welcome contributions to the FERS project! Whether you're interested in improving the C++ core, enhancing the UI, or
refining the documentation, your help is appreciated. Please read our [CONTRIBUTING.md](CONTRIBUTING.md)
guide to get started.

## License

- Copyright (C) 2006-2008 Marc Brooker and Michael Inggs.
- Copyright (C) 2008-present FERS contributors (see [AUTHORS.md](AUTHORS.md)).

This program is free software; you can redistribute it and/or modify it under the terms of the
[GNU General Public License](https://github.com/the-user-created/FERS/blob/master/LICENSE) as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the [GNU General Public License](https://github.com/the-user-created/FERS/blob/master/LICENSE) for
more details.

You should have received a copy of the [GNU General Public License](https://github.com/the-user-created/FERS/blob/master/LICENSE) along with this program; if not, write to
the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

### User-Generated Files

Please note that this license only covers the source code, program binaries, and build system of FERS. Any input files
you create (such as simulation scenarios) and any results generated by the simulator are not covered by this license and
remain the copyright of their original author.

### Third-Party Libraries

FERS incorporates code from the following third-party libraries, which are provided under their own licenses. The full
text for these licenses can be found in the `THIRD_PARTY_LICENSES` directory.

* **libxml2:** Used for XML parsing. Licensed under the [MIT License](THIRD_PARTY_LICENSES/libxml2-LICENSE.txt).
* **HighFive:** A C++ header-only library for HDF5. Licensed under
  the [Boost Software License 1.0](THIRD_PARTY_LICENSES/HighFive-LICENSE.txt).
* **GeographicLib:** A library for geographic calculations. Licensed under
  the [MIT License](THIRD_PARTY_LICENSES/GeographicLib-LICENSE.txt).
* **libhdf5:** Used for HDF5 file handling. Licensed under
  the [BSD 3-Clause License](THIRD_PARTY_LICENSES/libhdf5-LICENSE.txt).

### Historical Notice from Original Distribution

The following notice was part of the original FERS distribution:
> Should you wish to acquire a copy of FERS not covered by these terms, please contact the Department of
> Electrical Engineering at the University of Cape Town.
