// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { create } from 'zustand';
import { immer } from 'zustand/middleware/immer';
import { v4 as uuidv4 } from 'uuid';

// --- TYPE DEFINITIONS (Based on fers-xml.xsd) ---

export interface GlobalParameters {
    id: 'global-parameters';
    type: 'GlobalParameters';
    simulation_name: string;
    start: number;
    end: number;
    rate: number;
    c: number;
    random_seed: number | null;
    adc_bits: number;
    oversample_ratio: number;
    export: {
        xml: boolean;
        csv: boolean;
        binary: boolean; // h5
    };
}

export interface Pulse {
    id: string;
    type: 'Pulse';
    name: string;
    pulseType: 'file' | 'cw';
    power: number;
    carrier: number;
    filename?: string;
}

export interface Timing {
    id: string;
    type: 'Timing';
    name: string;
    frequency: number;
}

export interface Antenna {
    id: string;
    type: 'Antenna';
    name: string;
    pattern: 'sinc' | 'gaussian' | 'squarehorn' | 'parabolic' | 'xml' | 'file';
    filename?: string;
}

export interface PositionWaypoint {
    id: string;
    x: number;
    y: number;
    altitude: number;
    time: number;
}

export interface MotionPath {
    interpolation: 'static' | 'linear' | 'cubic';
    waypoints: PositionWaypoint[];
}

export interface FixedRotation {
    type: 'fixed';
    startAzimuth: number;
    startElevation: number;
    azimuthRate: number;
    elevationRate: number;
}

export interface RotationWaypoint {
    id: string;
    azimuth: number;
    elevation: number;
    time: number;
}

export interface RotationPath {
    type: 'path';
    interpolation: 'static' | 'linear' | 'cubic';
    waypoints: RotationWaypoint[];
}

export type PlatformComponent =
    | { type: 'none' }
    | {
    type: 'monostatic';
    name: string;
    radarType: 'pulsed' | 'cw';
    window_skip: number;
    window_length: number;
    prf: number;
    antennaId: string | null;
    pulseId: string | null;
    timingId: string | null;
}
    | {
    type: 'transmitter';
    name: string;
    radarType: 'pulsed' | 'cw';
    prf: number;
    antennaId: string | null;
    pulseId: string | null;
    timingId: string | null;
}
    | {
    type: 'receiver';
    name: string;
    window_skip: number;
    window_length: number;
    prf: number;
    antennaId: string | null;
    timingId: string | null;
}
    | {
    type: 'target';
    name: string;
    rcs_type: 'isotropic' | 'file';
    rcs_value?: number;
    rcs_filename?: string;
    rcs_model: 'constant' | 'chisquare' | 'gamma';
    rcs_k?: number;
};

export interface Platform {
    id: string;
    type: 'Platform';
    name: string;
    motionPath: MotionPath;
    rotation: FixedRotation | RotationPath;
    component: PlatformComponent;
}

export type ScenarioItem =
    | GlobalParameters
    | Pulse
    | Timing
    | Antenna
    | Platform;

// --- ZUSTAND STORE DEFINITION ---

type ScenarioState = {
    globalParameters: GlobalParameters;
    pulses: Pulse[];
    timings: Timing[];
    antennas: Antenna[];
    platforms: Platform[];
    selectedItemId: string | null;
};

type ScenarioActions = {
    selectItem: (itemId: string | null) => void;
    // Add/Remove Actions
    addPulse: () => void;
    addTiming: () => void;
    addAntenna: () => void;
    addPlatform: () => void;
    removeSelectedItem: () => void;
    // Waypoint Actions
    addPositionWaypoint: (platformId: string) => void;
    removePositionWaypoint: (platformId: string, waypointId: string) => void;
    addRotationWaypoint: (platformId: string) => void;
    removeRotationWaypoint: (platformId: string, waypointId: string) => void;
    // Update Action
    updateItem: (itemId: string, propertyPath: string, value: unknown) => void;
    setPlatformComponentType: (
        platformId: string,
        componentType: PlatformComponent['type']
    ) => void;
};

// --- DEFAULTS ---
const defaultGlobalParameters: GlobalParameters = {
    id: 'global-parameters',
    type: 'GlobalParameters',
    simulation_name: 'FERS Simulation',
    start: 0.0,
    end: 10.0,
    rate: 10000.0,
    c: 299792458.0,
    random_seed: null,
    adc_bits: 12,
    oversample_ratio: 1,
    export: {
        xml: false,
        csv: false,
        binary: true,
    },
};

const defaultPulse: Omit<Pulse, 'id' | 'name'> = {
    type: 'Pulse',
    pulseType: 'file',
    power: 1000,
    carrier: 1e9,
};

const defaultTiming: Omit<Timing, 'id' | 'name'> = {
    type: 'Timing',
    frequency: 10e6,
};

const defaultAntenna: Omit<Antenna, 'id' | 'name'> = {
    type: 'Antenna',
    pattern: 'sinc',
};

const defaultPlatform: Omit<Platform, 'id' | 'name'> = {
    type: 'Platform',
    motionPath: {
        interpolation: 'static',
        waypoints: [{ id: uuidv4(), x: 0, y: 0, altitude: 0, time: 0 }],
    },
    rotation: {
        type: 'fixed',
        startAzimuth: 0,
        startElevation: 0,
        azimuthRate: 0,
        elevationRate: 0,
    },
    component: {
        type: 'monostatic',
        name: 'Monostatic Radar', // Generic name, updated on creation
        radarType: 'pulsed',
        window_skip: 0,
        window_length: 0,
        prf: 1000,
        antennaId: null,
        pulseId: null,
        timingId: null,
    },
};

// Helper to set nested properties safely
const setPropertyByPath = (obj: object, path: string, value: unknown) => {
    const keys = path.split('.');
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    let current: any = obj;
    for (let i = 0; i < keys.length - 1; i++) {
        if (current[keys[i]] === undefined) return; // Path does not exist
        current = current[keys[i]];
    }
    current[keys[keys.length - 1]] = value;
};

export const useScenarioStore = create<ScenarioState & ScenarioActions>()(
    immer((set) => ({
        globalParameters: defaultGlobalParameters,
        pulses: [],
        timings: [],
        antennas: [],
        platforms: [],
        selectedItemId: null,

        selectItem: (itemId) => set({ selectedItemId: itemId }),

        addPulse: () =>
            set((state) => {
                const id = uuidv4();
                state.pulses.push({
                    ...defaultPulse,
                    id,
                    name: `Pulse ${state.pulses.length + 1}`,
                });
            }),
        addTiming: () =>
            set((state) => {
                const id = uuidv4();
                state.timings.push({
                    ...defaultTiming,
                    id,
                    name: `Timing ${state.timings.length + 1}`,
                });
            }),
        addAntenna: () =>
            set((state) => {
                const id = uuidv4();
                state.antennas.push({
                    ...defaultAntenna,
                    id,
                    name: `Antenna ${state.antennas.length + 1}`,
                });
            }),
        addPlatform: () =>
            set((state) => {
                const id = uuidv4();
                const newName = `Platform ${state.platforms.length + 1}`;
                const newPlatform: Platform = {
                    ...defaultPlatform,
                    id,
                    name: newName,
                };

                // Update the default component's name to match the new platform's name
                if (newPlatform.component.type === 'monostatic') {
                    newPlatform.component.name = `${newName} Monostatic`;
                }

                state.platforms.push(newPlatform);
            }),

        removeSelectedItem: () =>
            set((state) => {
                const id = state.selectedItemId;
                if (!id) return;

                const collections = [
                    'pulses',
                    'timings',
                    'antennas',
                    'platforms',
                ] as const;
                for (const key of collections) {
                    const index = state[key].findIndex(
                        (item: { id: string }) => item.id === id
                    );
                    if (index > -1) {
                        state[key].splice(index, 1);
                        state.selectedItemId = null;
                        return;
                    }
                }
            }),

        addPositionWaypoint: (platformId) =>
            set((state) => {
                const platform = state.platforms.find(
                    (p) => p.id === platformId
                );
                if (platform) {
                    platform.motionPath.waypoints.push({
                        id: uuidv4(),
                        x: 0,
                        y: 0,
                        altitude: 0,
                        time: 0,
                    });
                }
            }),

        removePositionWaypoint: (platformId, waypointId) =>
            set((state) => {
                const platform = state.platforms.find(
                    (p) => p.id === platformId
                );
                if (platform && platform.motionPath.waypoints.length > 1) {
                    const index = platform.motionPath.waypoints.findIndex(
                        (wp) => wp.id === waypointId
                    );
                    if (index > -1)
                        platform.motionPath.waypoints.splice(index, 1);
                }
            }),

        addRotationWaypoint: (platformId) =>
            set((state) => {
                const platform = state.platforms.find(
                    (p) => p.id === platformId
                );
                if (platform && platform.rotation.type === 'path') {
                    platform.rotation.waypoints.push({
                        id: uuidv4(),
                        azimuth: 0,
                        elevation: 0,
                        time: 0,
                    });
                }
            }),

        removeRotationWaypoint: (platformId, waypointId) =>
            set((state) => {
                const platform = state.platforms.find(
                    (p) => p.id === platformId
                );
                if (
                    platform &&
                    platform.rotation.type === 'path' &&
                    platform.rotation.waypoints.length > 1
                ) {
                    const index = platform.rotation.waypoints.findIndex(
                        (wp) => wp.id === waypointId
                    );
                    if (index > -1)
                        platform.rotation.waypoints.splice(index, 1);
                }
            }),

        updateItem: (itemId, propertyPath, value) =>
            set((state) => {
                if (itemId === 'global-parameters') {
                    setPropertyByPath(
                        state.globalParameters,
                        propertyPath,
                        value
                    );
                    return;
                }
                const collections = [
                    'pulses',
                    'timings',
                    'antennas',
                    'platforms',
                ] as const;
                for (const key of collections) {
                    const item = state[key].find(
                        (i: { id: string }) => i.id === itemId
                    );
                    if (item) {
                        setPropertyByPath(item, propertyPath, value);
                        return;
                    }
                }
            }),

        setPlatformComponentType: (platformId, componentType) =>
            set((state) => {
                const platform = state.platforms.find(
                    (p) => p.id === platformId
                );
                if (!platform || componentType === 'none') return;

                const name = `${platform.name} ${componentType}`;
                switch (componentType) {
                    case 'monostatic':
                        platform.component = {
                            type: 'monostatic',
                            name,
                            radarType: 'pulsed',
                            window_skip: 0,
                            window_length: 0,
                            prf: 1000,
                            antennaId: null,
                            pulseId: null,
                            timingId: null,
                        };
                        break;
                    case 'transmitter':
                        platform.component = {
                            type: 'transmitter',
                            name,
                            radarType: 'pulsed',
                            prf: 1000,
                            antennaId: null,
                            pulseId: null,
                            timingId: null,
                        };
                        break;
                    case 'receiver':
                        platform.component = {
                            type: 'receiver',
                            name,
                            window_skip: 0,
                            window_length: 0,
                            prf: 1000,
                            antennaId: null,
                            timingId: null,
                        };
                        break;
                    case 'target':
                        platform.component = {
                            type: 'target',
                            name,
                            rcs_type: 'isotropic',
                            rcs_value: 1,
                            rcs_model: 'constant',
                        };
                        break;
                }
            }),
    }))
);

// Helper function to find any item in the store by its ID
export const findItemInStore = (id: string | null): ScenarioItem | null => {
    if (!id) return null;
    const state = useScenarioStore.getState();
    if (id === 'global-parameters') return state.globalParameters;

    const collections = [
        state.pulses,
        state.timings,
        state.antennas,
        state.platforms,
    ];
    for (const collection of collections) {
        const item = collection.find((i) => i.id === id);
        if (item) return item as ScenarioItem;
    }
    return null;
};
