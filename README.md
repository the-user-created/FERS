# FERS: The Flexible Extensible Radar Simulator

<!-- TODO: Build status should be present for both ui and core -->
[![FERS Core CI](https://github.com/the-user-created/FERS/actions/workflows/CMake.yml/badge.svg)](https://github.com/the-user-created/FERS/actions/workflows/CMake.yml)
[![GitHub issues](https://img.shields.io/github/issues/the-user-created/FERS.svg)](https://github.com/the-user-created/FERS/issues)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://opensource.org/licenses/GPL-3.0)

![Tech Stack](https://img.shields.io/badge/Core-C%2B%2B%2023-00599C?logo=cplusplus)
![Tech Stack](https://img.shields.io/badge/UI-Tauri%20%7C%20React-20232A?logo=react)

FERS is a comprehensive suite of tools for signal-level radar simulation. It consists of a high-performance C++
simulation engine and a modern, intuitive graphical user interface for building and visualizing complex scenarios.

This repository is structured as a monorepo, containing the core simulator, the UI, and the shared data schema as
semi-independent packages.

---

## Key Features

- **High-Performance Core:** A modernized C++23 engine capable of detailed signal-level simulation with optimized
  multithreading.
- **Visual Scenario Builder:** An intuitive 3D interface to construct, configure, and visualize radar scenarios.
- **Flexible System Modeling:** Simulate a wide range of radar systems, including monostatic, multistatic, pulsed, and
  continuous wave (CW).
- **Advanced Data Export:** Output simulation data in HDF5, CSV, and XML formats for analysis.
- **Geographic Visualization:** Generate KML files from scenarios for accurate visualization in tools like Google Earth.
- **Unified Schema:** A central XML schema ensures consistency and serves as the single source of truth for scenarios
  across the simulator and the UI.

## Repository Structure

This monorepo contains the following packages:

- `packages/fers`: The core C++ radar simulation engine. This is a command-line tool that reads an XML scenario file
  and produces simulation output data.
- `packages/fers-ui`: A modern desktop application built with Tauri and React. It provides a graphical interface for
  creating, editing, and visualizing FERS scenarios.
- `packages/schema`: The XML Schema Definition (XSD) and Document Type Definition (DTD) that define the structure of
  FERS scenario files. This schema is the contract between the UI and the core simulator.

## Getting Started

To get started with a specific package, please refer to its dedicated README file for detailed instructions on
dependencies, building, and usage.

- **For the core simulator engine, see:** [`packages/fers/README.md`](packages/fers/README.md)
- **For the graphical user interface, see:** [`packages/fers-ui/README.md`](packages/fers-ui/README.md)

## Contributing

We welcome contributions to the FERS project! Whether you're interested in improving the C++ core, enhancing the UI, or
refining the documentation, your help is appreciated. Please read our [CONTRIBUTING.md](.github/CONTRIBUTING.md)
guide to get started.

## License

FERS is licensed under the GNU General Public License v3.0. See the [LICENSE](LICENSE) file for more details.
