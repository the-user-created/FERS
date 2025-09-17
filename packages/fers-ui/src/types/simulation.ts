// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

/**
 * Base interface for any element in the simulation scene.
 */
export interface SimulationElement {
    id: string;
    name: string;
    type: 'Platform' | 'Target' | 'Antenna' | 'Pulse';
    parentId?: string | null;
    // Add other common properties here
}

/**
 * Represents a platform, which can be a sensor, a target, etc.
 */
export interface Platform extends SimulationElement {
    type: 'Platform';
    position: { x: number; y: number; z: number };
    // Platform-specific properties
}

/**
 * Represents a target to be detected.
 */
export interface Target extends SimulationElement {
    type: 'Target';
    rcs: number; // Radar Cross-Section
    // Target-specific properties
}

/**
 * Represents an antenna on a platform.
 */
export interface Antenna extends SimulationElement {
    type: 'Antenna';
    // Antenna-specific properties (e.g., pattern, gain)
}

/**
 * Represents a transmitted pulse or waveform.
 */
export interface Pulse extends SimulationElement {
    type: 'Pulse';
    // Pulse-specific properties (e.g., bandwidth, duration)
}

// A union type representing any possible element in the simulation.
export type AnySimulationElement = Platform | Target | Antenna | Pulse;
