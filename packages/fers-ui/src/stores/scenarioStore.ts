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
    simSamplingRate: number | null;
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

export interface NoiseEntry {
    id: string;
    alpha: number;
    weight: number;
}

export interface Timing {
    id: string;
    type: 'Timing';
    name: string;
    frequency: number;
    freqOffset: number | null;
    randomFreqOffsetStdev: number | null;
    phaseOffset: number | null;
    randomPhaseOffsetStdev: number | null;
    noiseEntries: NoiseEntry[];
}

export interface Antenna {
    id: string;
    type: 'Antenna';
    name: string;
    pattern:
        | 'isotropic'
        | 'sinc'
        | 'gaussian'
        | 'squarehorn'
        | 'parabolic'
        | 'xml'
        | 'file';
    filename?: string;
    efficiency: number | null;
    // sinc
    alpha?: number | null;
    beta?: number | null;
    gamma?: number | null;
    // gaussian
    azscale?: number | null;
    elscale?: number | null;
    // squarehorn, parabolic
    diameter?: number | null;
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
          noiseTemperature: number | null;
          noDirectPaths: boolean;
          noPropagationLoss: boolean;
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
          noiseTemperature: number | null;
          noDirectPaths: boolean;
          noPropagationLoss: boolean;
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
// --- ZUSTAND STORE DEFINITION ---

/** The data structure that is imported from/exported to XML */
export type ScenarioData = {
    globalParameters: GlobalParameters;
    pulses: Pulse[];
    timings: Timing[];
    antennas: Antenna[];
    platforms: Platform[];
};

export type ScenarioItem =
    | GlobalParameters
    | Pulse
    | Timing
    | Antenna
    | Platform;
type ScenarioState = ScenarioData & {
    selectedItemId: string | null;
};

type ScenarioActions = {
    selectItem: (itemId: string | null) => void;
    // Add/Remove Actions
    addPulse: () => void;
    addTiming: () => void;
    addAntenna: () => void;
    addPlatform: () => void;
    removeItem: (itemId: string) => void;
    // Waypoint Actions
    addPositionWaypoint: (platformId: string) => void;
    removePositionWaypoint: (platformId: string, waypointId: string) => void;
    addRotationWaypoint: (platformId: string) => void;
    removeRotationWaypoint: (platformId: string, waypointId: string) => void;
    addNoiseEntry: (timingId: string) => void;
    removeNoiseEntry: (timingId: string, entryId: string) => void;
    setAntennaPattern: (
        antennaId: string,
        newPattern: Antenna['pattern']
    ) => void;
    // Update Action
    updateItem: (itemId: string, propertyPath: string, value: unknown) => void;
    setPlatformComponentType: (
        platformId: string,
        componentType: PlatformComponent['type']
    ) => void;
    loadScenario: (newData: ScenarioData) => void;
};

// --- DEFAULTS ---
const defaultGlobalParameters: GlobalParameters = {
    id: 'global-parameters',
    type: 'GlobalParameters',
    simulation_name: 'FERS Simulation',
    start: 0.0,
    end: 10.0,
    rate: 10000.0,
    simSamplingRate: null,
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
    freqOffset: null,
    randomFreqOffsetStdev: null,
    phaseOffset: null,
    randomPhaseOffsetStdev: null,
    noiseEntries: [],
};

const defaultAntenna: Omit<Antenna, 'id' | 'name'> = {
    type: 'Antenna',
    pattern: 'isotropic',
    efficiency: 1.0,
    alpha: 1.0,
    beta: 1.0,
    gamma: 0.0,
    azscale: null,
    elscale: null,
    diameter: null,
};

const createDefaultPlatform = (): Omit<Platform, 'id' | 'name'> => ({
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
        noiseTemperature: 290,
        noDirectPaths: false,
        noPropagationLoss: false,
    },
});

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
        scenarioId: uuidv4(), // A unique ID for the scenario instance
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
                    ...createDefaultPlatform(),
                    id,
                    name: newName,
                };

                // Update the default component's name to match the new platform's name
                if (newPlatform.component.type === 'monostatic') {
                    newPlatform.component.name = `${newName} Monostatic`;
                }

                state.platforms.push(newPlatform);
            }),

        removeItem: (itemId) =>
            set((state) => {
                if (!itemId) return;

                const collections = [
                    'pulses',
                    'timings',
                    'antennas',
                    'platforms',
                ] as const;
                for (const key of collections) {
                    const index = state[key].findIndex(
                        (item: { id: string }) => item.id === itemId
                    );
                    if (index > -1) {
                        state[key].splice(index, 1);
                        if (state.selectedItemId === itemId) {
                            state.selectedItemId = null;
                        }
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
        addNoiseEntry: (timingId) =>
            set((state) => {
                const timing = state.timings.find((t) => t.id === timingId);
                if (timing) {
                    timing.noiseEntries.push({
                        id: uuidv4(),
                        alpha: 0,
                        weight: 0,
                    });
                }
            }),
        removeNoiseEntry: (timingId, entryId) =>
            set((state) => {
                const timing = state.timings.find((t) => t.id === timingId);
                if (timing) {
                    const index = timing.noiseEntries.findIndex(
                        (e) => e.id === entryId
                    );
                    if (index > -1) {
                        timing.noiseEntries.splice(index, 1);
                    }
                }
            }),
        setAntennaPattern: (antennaId, newPattern) =>
            set((state) => {
                const antenna = state.antennas.find((a) => a.id === antennaId);
                if (antenna) {
                    antenna.pattern = newPattern;
                    // Reset all pattern-specific fields
                    antenna.alpha = null;
                    antenna.beta = null;
                    antenna.gamma = null;
                    antenna.azscale = null;
                    antenna.elscale = null;
                    antenna.diameter = null;
                    antenna.filename = undefined;

                    // Set defaults for the new pattern
                    switch (newPattern) {
                        case 'sinc':
                            antenna.alpha = 1.0;
                            antenna.beta = 1.0;
                            antenna.gamma = 0.0;
                            break;
                        case 'gaussian':
                            antenna.azscale = 1.0;
                            antenna.elscale = 1.0;
                            break;
                        case 'squarehorn':
                        case 'parabolic':
                            antenna.diameter = 0.5;
                            break;
                    }
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
                            noiseTemperature: 290,
                            noDirectPaths: false,
                            noPropagationLoss: false,
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
                            noiseTemperature: 290,
                            noDirectPaths: false,
                            noPropagationLoss: false,
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
        loadScenario: (newData) =>
            set((state) => {
                console.log('[DEBUG] Store state BEFORE loadScenario:', {
                    ...state,
                });

                state.globalParameters = {
                    ...newData.globalParameters,
                    id: 'global-parameters',
                    type: 'GlobalParameters',
                };
                state.pulses = newData.pulses;
                state.timings = newData.timings;
                state.antennas = newData.antennas;
                state.platforms = newData.platforms;
                state.selectedItemId = null; // Deselect item on new load

                console.log('[DEBUG] Store state AFTER loadScenario:', {
                    ...state,
                });
            }),
    }))
);

// Helper function to find any item in the store by its ID
export const findItemInStore = (
    state: ScenarioState,
    id: string | null
): ScenarioItem | null => {
    if (!id) return null;
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
