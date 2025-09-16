# fers-ui: A Graphical Interface for the Flexible Extensible Radar Simulator

![MSc Project](https://img.shields.io/badge/Status-MSc%20Project-blue)
![Tauri Version](https://img.shields.io/badge/Tauri-v2-FFC336)
![Framework](https://img.shields.io/badge/Framework-React-61DAFB?logo=react)
![Language](https://img.shields.io/badge/Language-TypeScript-3178C6?logo=typescript)
![License](https://img.shields.io/badge/License-GPL--3.0-blue)

## 1. Introduction

`fers-ui` is the graphical user interface for the **Flexible Extensible Radar Simulator (FERS)**, a powerful,
open-source C++ based signal-level simulation tool.

This repository contains the development of a modern GUI for FERS, with the primary goal of dramatically improving its
usability, accessibility, and educational value. The UI aims to provide an intuitive, visual way to construct,
configure, and visualize complex radar simulation scenarios.

This project is being actively developed as part of a **Master of Science (MSc) in Electrical Engineering** at the
University of Cape Town.

---

## 2. Project Status & Technology Choices

### Technology Stack

* **Application Framework:** [Tauri](https://tauri.app/) (v2)
* **UI Library:** [React](https://react.dev/) (v19) with [TypeScript](https://www.typescriptlang.org/)
* **Component Library:** [Material-UI](https://mui.com/material-ui/) (v7)
* **3D Rendering:** [Three.js](https://threejs.org/) via [React Three Fiber](https://docs.pmnd.rs/react-three-fiber)
* **State Management:** [Zustand](https://docs.pmnd.rs/zustand)
* **Core Simulator:** FERS (C++23)
* **Data Interchange Format:** XML (for compatibility with FERS)

### Rationale for Tooling

The choice of a modern web technology stack provides a powerful combination of rapid development, cross-platform
compatibility, and a rich ecosystem.

1. **Tauri:** Provides a lightweight, secure, and performant way to build a native desktop application using web
   front-ends. It allows for deep system integration (like file system access) which is crucial for interacting with the
   FERS core engine and its configuration files.
2. **React & TypeScript:** Offers a robust, component-based architecture for building complex UIs. TypeScript adds
   static typing, which is invaluable for maintaining a large and complex application by catching errors early and
   improving code clarity.
3. **React Three Fiber (R3F):** A declarative, component-based renderer for Three.js. It simplifies managing a 3D scene
   within a React application, making it easier to create the **3D World Builder** and integrate it with the rest of the
   UI.
4. **Material-UI:** A comprehensive library of UI components that accelerates development and ensures a consistent,
   modern design language.
5. **Zustand:** A minimal, fast, and scalable state management solution. It's ideal for implementing the "single source
   of truth" architecture, providing a central store for simulation data that UI components can react to without
   performance overhead or boilerplate.

---

## 3. Core Features

The UI will provide a complete "workbench" for FERS, from scenario creation to results visualization.

*   [ ] **Visual 3D Scenario Builder:** Interactively place and configure platforms, targets, transmitters, and
    receivers in a 3D environment.
*   [ ] **Hierarchical Scene View:** A tree view of all elements in the simulation (platforms, antennas, pulses, etc.)
    for easy selection and organization.
*   [ ] **Dynamic Property Inspector:** A context-sensitive panel to edit the parameters of any selected simulation
    element.
*   [ ] **FERS XML Exporter:** Generate a valid FERS XML configuration file from the visual scenario.
*   [ ] **Dynamic Simulation Replay & Animation:** Load FERS output data to visualize target trajectories and platform
    movements.
*   [ ] **Integrated Asset Editors:** UI-driven tools for creating basic waveforms, defining target RCS, and specifying
    antenna patterns.

---

## 4. Software Architecture

The project is architected using modern software design principles to ensure it is maintainable, extensible, and robust.

### a. Centralized State Management (Single Source of Truth)

The UI's architecture is centered around a **Zustand store**, which acts as the single source of truth for all
simulation configuration data. This approach is directly inspired by the `SimulationData` singleton from the original
Godot prototype.

* **Decoupling:** UI components do not communicate directly. Instead, they read data from the Zustand store or call its
  actions to modify the state.
* **Reactivity:** Components subscribe to parts of the state and automatically re-render when that data changes. This
  creates a clean, reactive, and one-way data flow.

```tsx
// in RightSidebar.tsx (Scene Hierarchy)
const {selectElement} = useSimulationActions();
// ...
// NOTIFY the central store by calling an action.
return <ListItemButton onClick={() => selectElement(element.id)}/>;


// in LeftSidebar.tsx (Property Inspector)
// LISTEN to the central store for changes.
const selectedElementId = useSimulationStore((state) => state.selectedElementId);
// ... UI updates automatically when selectedElementId changes.
```

### b. Component-Based & Reusable UI

The UI is built from small, reusable, and single-purpose React components, leveraging Material-UI's library.

* **Property Editors:** The property inspector will dynamically render the correct editor component (`PulseEditor`,
  `PlatformEditor`, etc.) based on the selected element's type, using a factory or conditional rendering pattern.
* **Custom Hooks:** Business logic is encapsulated in custom hooks to be shared across components without repeating
  code, following the DRY principle.

### c. Data-Driven UI

The user interface is generated dynamically based on the state in the Zustand store, not hardcoded.

* The `PlatformEditor` component will intelligently show or hide fields based on the chosen platform type.
* Dropdowns for linking elements (e.g., assigning an `Antenna` to a `Platform`) will be populated at runtime by querying
  the store for all available elements of that type.

### d. Consistent Project Structure & Naming Conventions

The project adheres to community best practices for organizing a React/Tauri application.

* **Folder Structure:** A clear separation between `components`, `views`, `layouts`, `store`, and `types`.
* **Naming:** `PascalCase` is used for components and types, while `camelCase` is used for functions, variables, and
  hooks.

```
/src/
├── components/         # Reusable, generic UI components (e.g., WorldView)
├── layouts/            # Top-level layout structures (e.g., MainLayout)
├── store/              # Zustand state management
├── types/              # TypeScript type definitions
└── views/              # Major UI panels/sections (e.g., LeftSidebar)
```

### e. Adherence to the Single Responsibility Principle (SRP)

Each React component and hook has a well-defined and limited responsibility.

* `simulationStore.ts` is only concerned with managing the state of the simulation data.
* `WorldView.tsx` is only responsible for visualizing FERS elements in the 3D viewport.
* `LeftSidebar.tsx` is only responsible for displaying the properties of the selected element.

This separation of concerns makes the code easier to understand, debug, and extend.

---

## 5. Getting Started

1. Ensure you have [Node.js](https://nodejs.org/), [pnpm](https://pnpm.io/), and
   the [Rust toolchain](https://www.rust-lang.org/tools/install) installed.
2. Set up the [Tauri prerequisites](https://tauri.app/start/prerequisites/) for your operating
   system.
3. Clone this repository:
   ```bash
   git clone https://github.com/the-user-created/fers-ui.git
   ```
4. Navigate to the project directory and install dependencies:
   ```bash
   cd fers-ui
   pnpm install
   ```
5. Run the application in development mode:
   ```bash
   pnpm tauri dev
   ```
