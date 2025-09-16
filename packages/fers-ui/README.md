# fers-ui: A Graphical Interface for the FERS Core Simulator

![Framework](https://img.shields.io/badge/Framework-React-61DAFB?logo=react)
![Language](https://img.shields.io/badge/Language-TypeScript-3178C6?logo=typescript)
![Desktop App](https://img.shields.io/badge/Tauri-v2-FFC336)
[![License](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](https://opensource.org/licenses/GPL-3.0)

**fers-ui** is the official graphical user interface for the **Flexible Extensible Radar Simulator (FERS)**. It provides
an intuitive, visual workbench for constructing, configuring, and visualizing complex radar simulation scenarios,
dramatically improving the usability and accessibility of the core FERS engine.

## Planned Features

- **Visual 3D Scenario Builder:** Interactively place and configure platforms, targets, transmitters, and receivers in a
  3D environment.
- **Hierarchical Scene View:** A tree view of all elements in the simulation for easy selection and organization.
- **Dynamic Property Inspector:** A context-sensitive panel to edit the parameters of any selected simulation element.
- **FERS XML Exporter:** Generate a valid FERS XML configuration file from the visual scenario.
- **Dynamic Simulation Replay & Animation:** Load FERS output data to visualize target trajectories and platform
  movements.
- **Integrated Asset Editors:** UI-driven tools for creating basic waveforms, defining target RCS, and specifying
  antenna patterns.

## Technology Stack

fers-ui is built with a modern technology stack to provide a powerful, cross-platform, and maintainable application.

- **Application Framework:** [**Tauri**](https://tauri.app/) (v2) - Provides a lightweight, secure, and performant way
  to build a native desktop application using a web front-end.
- **UI Library:** [**React**](https://react.dev/) with [**TypeScript**](https://www.typescriptlang.org/) - Offers a
  robust, component-based architecture for building complex UIs with the safety of static typing.
- **Component Library:** [**Material-UI**](https://mui.com/material-ui/) - A comprehensive library of UI components that
  accelerates development and ensures a consistent, modern design.
- **3D Rendering:** [**Three.js**](https://threejs.org/) via [**React Three Fiber**](https://docs.pmnd.rs/react-three-fiber) - Simplifies managing a 3D scene within a React application, powering the
  visual scenario builder.
- **State Management:** [**Zustand**](https://docs.pmnd.rs/zustand) - A minimal, fast, and scalable state management
  solution that serves as the single source of truth for all simulation data.

## Software Architecture

The application is architected around modern design principles to ensure it is robust, maintainable, and extensible.

### Single Source of Truth

The UI's architecture is centered around a **Zustand store**, which acts as the single source of truth for all
simulation configuration data. UI components do not communicate directly; instead, they read from the store and dispatch
actions to modify state, creating a clean, reactive, and one-way data flow.

### Component-Based UI

The interface is built from small, reusable React components. Business logic is encapsulated in custom hooks to be
shared across components, adhering to the DRY (Don't Repeat Yourself) principle.

### Data-Driven Views

The UI is generated dynamically based on the state in the Zustand store. For example, the property inspector renders the
correct editor component (`PulseEditor`, `PlatformEditor`, etc.) based on the selected element's type, and dropdowns are
populated at runtime by querying the store.

### Project Structure

The project adheres to community best practices for organizing a React/Tauri application, with a clear separation of
concerns between `components`, `views`, `layouts`, `store`, and `types`.

## Getting Started

### Prerequisites

1. [**Node.js**](https://nodejs.org/) and [**pnpm**](https://pnpm.io/).
2. The [**Rust toolchain**](https://www.rust-lang.org/tools/install).
3. [**Tauri prerequisites**](https://tauri.app/start/prerequisites/) for your operating system.

### Installation & Running

1. **Clone the repository:** (If you haven't already)

    ```bash
    git clone https://github.com/the-user-created/FERS.git
    ```

2. **Navigate to the `fers-ui` package and install dependencies:**

    ```bash
    cd FERS/packages/fers-ui
    pnpm install
    ```

3. **Run the application in development mode:**
    ```bash
    pnpm tauri dev
    ```
