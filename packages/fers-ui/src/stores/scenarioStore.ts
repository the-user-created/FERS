// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { create } from 'zustand';
import { immer } from 'zustand/middleware/immer';
import { v4 as uuidv4 } from 'uuid';
import { invoke } from '@tauri-apps/api/core';
import { z } from 'zod';
import {
    GlobalParametersSchema,
    PulseSchema,
    TimingSchema,
    AntennaSchema,
    PlatformSchema,
    ScenarioDataSchema,
    NoiseEntrySchema,
    PositionWaypointSchema,
    RotationWaypointSchema,
    PlatformComponentSchema,
    FixedRotationSchema,
    RotationPathSchema,
    MotionPathSchema,
} from './scenarioSchema';

// --- TYPE DEFINITIONS ---

export type GlobalParameters = z.infer<typeof GlobalParametersSchema>;
export type Pulse = z.infer<typeof PulseSchema>;
export type NoiseEntry = z.infer<typeof NoiseEntrySchema>;
export type Timing = z.infer<typeof TimingSchema>;
export type Antenna = z.infer<typeof AntennaSchema>;
export type PositionWaypoint = z.infer<typeof PositionWaypointSchema>;
export type MotionPath = z.infer<typeof MotionPathSchema>;
export type FixedRotation = z.infer<typeof FixedRotationSchema>;
export type RotationWaypoint = z.infer<typeof RotationWaypointSchema>;
export type RotationPath = z.infer<typeof RotationPathSchema>;
export type PlatformComponent = z.infer<typeof PlatformComponentSchema>;
export type Platform = z.infer<typeof PlatformSchema>;

/** Extract the Target component type from the union for safer access */
export type TargetComponent = Extract<PlatformComponent, { type: 'target' }>;

/** The data structure that is imported from/exported to XML */
export type ScenarioData = z.infer<typeof ScenarioDataSchema>;

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
    setPlatformRcsModel: (
        platformId: string,
        newModel: TargetComponent['rcs_model']
    ) => void;
    loadScenario: (backendData: any) => void;
    // New actions for backend synchronization
    syncBackend: () => Promise<void>;
    fetchFromBackend: () => Promise<void>;
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
    origin: {
        latitude: -33.957652, // Default: UCT, South Africa
        longitude: 18.4611991,
        altitude: 111.01,
    },
    coordinateSystem: {
        frame: 'ENU',
    },
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
    filename: '',
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

const defaultAntenna: Omit<
    Extract<Antenna, { pattern: 'isotropic' }>,
    'id' | 'name'
> = {
    type: 'Antenna',
    pattern: 'isotropic',
    efficiency: 1.0,
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
        const key = keys[i];
        if (current[key] === undefined || current[key] === null) return; // Path does not exist
        current = current[key];
    }
    current[keys[keys.length - 1]] = value;
};

export const useScenarioStore = create<ScenarioState & ScenarioActions>()(
    immer((set, get) => ({
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
                if (platform?.rotation.type === 'path') {
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
                    platform?.rotation.type === 'path' &&
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
                    if (index > -1) timing.noiseEntries.splice(index, 1);
                }
            }),

        setAntennaPattern: (antennaId, newPattern) =>
            set((state) => {
                const antenna = state.antennas.find((a) => a.id === antennaId);
                if (!antenna) return;

                const baseAntenna: Omit<Antenna, 'pattern'> = { ...antenna };
                delete (baseAntenna as any).pattern; // Remove pattern to avoid discriminated union issues

                let newAntennaState: Antenna;

                switch (newPattern) {
                    case 'isotropic':
                        newAntennaState = {
                            ...baseAntenna,
                            pattern: 'isotropic',
                        };
                        break;
                    case 'sinc':
                        newAntennaState = {
                            ...baseAntenna,
                            pattern: 'sinc',
                            alpha: 1.0,
                            beta: 1.0,
                            gamma: 0.0,
                        };
                        break;
                    case 'gaussian':
                        newAntennaState = {
                            ...baseAntenna,
                            pattern: 'gaussian',
                            azscale: 1.0,
                            elscale: 1.0,
                        };
                        break;
                    case 'squarehorn':
                    case 'parabolic':
                        newAntennaState = {
                            ...baseAntenna,
                            pattern: newPattern,
                            diameter: 0.5,
                        };
                        break;
                    case 'xml':
                    case 'file':
                        newAntennaState = {
                            ...baseAntenna,
                            pattern: newPattern,
                            filename: '',
                        };
                        break;
                }
                const index = state.antennas.findIndex(
                    (a) => a.id === antennaId
                );
                if (index > -1) state.antennas[index] = newAntennaState;
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
                if (!platform) return;

                const name = `${platform.name} ${componentType}`;
                let newComponent: PlatformComponent;

                switch (componentType) {
                    case 'monostatic':
                        newComponent = {
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
                        newComponent = {
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
                        newComponent = {
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
                        newComponent = {
                            type: 'target',
                            name,
                            rcs_type: 'isotropic',
                            rcs_value: 1,
                            rcs_model: 'constant',
                        };
                        break;
                    case 'none':
                        newComponent = { type: 'none' };
                        break;
                }
                platform.component = newComponent;
            }),

        setPlatformRcsModel: (platformId, newModel) =>
            set((state) => {
                const platform = state.platforms.find(
                    (p) => p.id === platformId
                );
                if (platform?.component.type === 'target') {
                    platform.component.rcs_model = newModel;
                    if (newModel === 'chisquare' || newModel === 'gamma') {
                        if (typeof platform.component.rcs_k !== 'number') {
                            platform.component.rcs_k = 1.0;
                        }
                    } else {
                        delete platform.component.rcs_k;
                    }
                }
            }),

        loadScenario: (backendData: any) => {
            try {
                const data = backendData.simulation || backendData;
                const nameToIdMap = new Map<string, string>();
                const assignId = (item: any) => ({ ...item, id: uuidv4() });

                // 1. Parameters
                const params = data.parameters || {};
                const globalParameters: GlobalParameters = {
                    id: 'global-parameters',
                    type: 'GlobalParameters',
                    simulation_name: data.name || 'FERS Simulation',
                    start: params.starttime ?? 0.0,
                    end: params.endtime ?? 10.0,
                    rate: params.rate ?? 10000.0,
                    simSamplingRate: params.simSamplingRate ?? null,
                    c: params.c ?? 299792458.0,
                    random_seed: params.randomseed ?? null,
                    adc_bits: params.adc_bits ?? 12,
                    oversample_ratio: params.oversample ?? 1,
                    origin: {
                        latitude: params.origin?.latitude ?? -33.957652,
                        longitude: params.origin?.longitude ?? 18.4611991,
                        altitude: params.origin?.altitude ?? 111.01,
                    },
                    coordinateSystem: {
                        frame: params.coordinatesystem?.frame ?? 'ENU',
                        zone: params.coordinatesystem?.zone,
                        hemisphere: params.coordinatesystem?.hemisphere,
                    },
                    export: {
                        xml: params.export?.xml ?? false,
                        csv: params.export?.csv ?? false,
                        binary: params.export?.binary ?? true,
                    },
                };

                // 2. Assets (and build name-to-id map)
                const pulses: Pulse[] = (data.pulses || []).map((p: any) => {
                    const pulse = {
                        ...assignId(p),
                        type: 'Pulse',
                        pulseType: p.type === 'continuous' ? 'cw' : 'file',
                        filename: p.filename ?? '',
                    };
                    nameToIdMap.set(pulse.name, pulse.id);
                    return pulse;
                });

                const timings: Timing[] = (data.timings || []).map((t: any) => {
                    const timing = {
                        ...assignId(t),
                        type: 'Timing',
                        freqOffset: t.freq_offset ?? null,
                        randomFreqOffsetStdev:
                            t.random_freq_offset_stdev ?? null,
                        phaseOffset: t.phase_offset ?? null,
                        randomPhaseOffsetStdev:
                            t.random_phase_offset_stdev ?? null,
                        noiseEntries: (t.noise_entries || []).map(assignId),
                    };
                    nameToIdMap.set(timing.name, timing.id);
                    return timing;
                });

                const antennas: Antenna[] = (data.antennas || []).map(
                    (a: any) => {
                        const antenna: any = {
                            ...assignId(a),
                            type: 'Antenna',
                        };
                        nameToIdMap.set(antenna.name, antenna.id);
                        return antenna;
                    }
                );

                // 3. Platforms
                const platforms: Platform[] = (data.platforms || []).map(
                    (p: any): Platform => {
                        const motionPath: MotionPath = {
                            interpolation:
                                p.motionpath?.interpolation ?? 'static',
                            waypoints: (
                                p.motionpath?.positionwaypoints ?? []
                            ).map(assignId),
                        };

                        let rotation: FixedRotation | RotationPath;
                        if (p.fixedrotation) {
                            rotation = {
                                type: 'fixed',
                                startAzimuth: p.fixedrotation.startazimuth,
                                startElevation: p.fixedrotation.startelevation,
                                azimuthRate: p.fixedrotation.azimuthrate,
                                elevationRate: p.fixedrotation.elevationrate,
                            };
                        } else if (p.rotationpath) {
                            rotation = {
                                type: 'path',
                                interpolation:
                                    p.rotationpath.interpolation ?? 'static',
                                waypoints: (
                                    p.rotationpath.rotationwaypoints || []
                                ).map(assignId),
                            };
                        } else {
                            rotation = createDefaultPlatform().rotation as
                                | FixedRotation
                                | RotationPath;
                        }

                        let component: PlatformComponent = { type: 'none' };
                        if (
                            p.component &&
                            Object.keys(p.component).length > 0
                        ) {
                            const cType = Object.keys(p.component)[0];
                            const cData = p.component[cType];
                            const commonRadar = {
                                antennaId:
                                    nameToIdMap.get(cData.antenna) ?? null,
                                timingId: nameToIdMap.get(cData.timing) ?? null,
                            };
                            const commonReceiver = {
                                noiseTemperature: cData.noise_temp ?? null,
                                noDirectPaths: cData.nodirect ?? false,
                                noPropagationLoss:
                                    cData.nopropagationloss ?? false,
                            };

                            switch (cType) {
                                case 'monostatic':
                                    component = {
                                        type: 'monostatic',
                                        name: cData.name,
                                        radarType:
                                            cData.type === 'cw'
                                                ? 'cw'
                                                : 'pulsed',
                                        window_skip: cData.window_skip ?? 0,
                                        window_length: cData.window_length ?? 0,
                                        prf: cData.prf ?? 1000,
                                        pulseId:
                                            nameToIdMap.get(cData.pulse) ??
                                            null,
                                        ...commonRadar,
                                        ...commonReceiver,
                                    };
                                    break;
                                case 'transmitter':
                                    component = {
                                        type: 'transmitter',
                                        name: cData.name,
                                        radarType:
                                            cData.type === 'cw'
                                                ? 'cw'
                                                : 'pulsed',
                                        prf: cData.prf ?? 1000,
                                        pulseId:
                                            nameToIdMap.get(cData.pulse) ??
                                            null,
                                        ...commonRadar,
                                    };
                                    break;
                                case 'receiver':
                                    component = {
                                        type: 'receiver',
                                        name: cData.name,
                                        window_skip: cData.window_skip ?? 0,
                                        window_length: cData.window_length ?? 0,
                                        prf: cData.prf ?? 1000,
                                        ...commonRadar,
                                        ...commonReceiver,
                                    };
                                    break;
                                case 'target':
                                    component = {
                                        type: 'target',
                                        name: cData.name,
                                        rcs_type:
                                            cData.rcs?.type ?? 'isotropic',
                                        rcs_value: cData.rcs?.value,
                                        rcs_filename: cData.rcs?.filename,
                                        rcs_model:
                                            cData.model?.type ?? 'constant',
                                        rcs_k: cData.model?.k,
                                    };
                                    break;
                            }
                        }

                        return {
                            id: uuidv4(),
                            type: 'Platform',
                            name: p.name,
                            motionPath,
                            rotation,
                            component,
                        };
                    }
                );

                const transformedScenario: ScenarioData = {
                    globalParameters,
                    pulses,
                    timings,
                    antennas,
                    platforms,
                };

                // --- Validate and Commit ---
                const result =
                    ScenarioDataSchema.safeParse(transformedScenario);

                if (!result.success) {
                    console.error(
                        'Data validation failed after loading from backend:',
                        result.error.flatten()
                    );
                    // Optionally, show an error notification to the user here.
                    return;
                }

                // Update state with the validated and parsed data.
                set({
                    ...result.data,
                    selectedItemId: null,
                });
            } catch (error) {
                console.error(
                    'An unexpected error occurred while loading the scenario:',
                    error
                );
            }
        },

        /** Pushes the current Zustand state to the C++ backend */
        syncBackend: async () => {
            const { globalParameters, pulses, timings, antennas, platforms } =
                get();

            // Helper functions to map frontend asset IDs back to names for the backend
            const findAntennaName = (id: string | null) =>
                antennas.find((a) => a.id === id)?.name;
            const findPulseName = (id: string | null) =>
                pulses.find((p) => p.id === id)?.name;
            const findTimingName = (id: string | null) =>
                timings.find((t) => t.id === id)?.name;

            // Helper to strip null/undefined values from an object before sending to backend
            const cleanObject = (obj: any): any => {
                if (Array.isArray(obj)) {
                    return obj.map(cleanObject);
                }
                if (obj !== null && typeof obj === 'object') {
                    return Object.entries(obj).reduce((acc, [key, value]) => {
                        if (value !== null && value !== undefined) {
                            acc[key] = cleanObject(value);
                        }
                        return acc;
                    }, {} as any);
                }
                return obj;
            };

            const backendPlatforms = platforms.map((p) => {
                const { component, motionPath, rotation, id, type, ...rest } =
                    p;

                let backendComponent = {};
                if (component.type !== 'none') {
                    switch (component.type) {
                        case 'monostatic':
                            backendComponent = {
                                monostatic: {
                                    name: component.name,
                                    type: component.radarType,
                                    window_skip: component.window_skip,
                                    window_length: component.window_length,
                                    prf: component.prf,
                                    antenna: findAntennaName(
                                        component.antennaId
                                    ),
                                    pulse: findPulseName(component.pulseId),
                                    timing: findTimingName(component.timingId),
                                    noise_temp: component.noiseTemperature,
                                    nodirect: component.noDirectPaths,
                                    nopropagationloss:
                                        component.noPropagationLoss,
                                },
                            };
                            break;
                        case 'transmitter':
                            backendComponent = {
                                transmitter: {
                                    name: component.name,
                                    type: component.radarType,
                                    prf: component.prf,
                                    antenna: findAntennaName(
                                        component.antennaId
                                    ),
                                    pulse: findPulseName(component.pulseId),
                                    timing: findTimingName(component.timingId),
                                },
                            };
                            break;
                        case 'receiver':
                            backendComponent = {
                                receiver: {
                                    name: component.name,
                                    window_skip: component.window_skip,
                                    window_length: component.window_length,
                                    prf: component.prf,
                                    antenna: findAntennaName(
                                        component.antennaId
                                    ),
                                    timing: findTimingName(component.timingId),
                                    noise_temp: component.noiseTemperature,
                                    nodirect: component.noDirectPaths,
                                    nopropagationloss:
                                        component.noPropagationLoss,
                                },
                            };
                            break;
                        case 'target':
                            {
                                const targetObj: any = {
                                    name: component.name,
                                    rcs: {
                                        type: component.rcs_type,
                                        value: component.rcs_value,
                                        filename: component.rcs_filename,
                                    },
                                };
                                if (component.rcs_model !== 'constant') {
                                    targetObj.model = {
                                        type: component.rcs_model,
                                        k: component.rcs_k,
                                    };
                                }
                                backendComponent = { target: targetObj };
                            }
                            break;
                    }
                }

                const backendRotation: any = {};
                if (rotation.type === 'fixed') {
                    const { type, ...r } = rotation;
                    backendRotation.fixedrotation = {
                        interpolation: 'constant',
                        startazimuth: r.startAzimuth,
                        startelevation: r.startElevation,
                        azimuthrate: r.azimuthRate,
                        elevationrate: r.elevationRate,
                    };
                } else {
                    const { type, ...r } = rotation;
                    backendRotation.rotationpath = {
                        interpolation: r.interpolation,
                        rotationwaypoints: r.waypoints.map(
                            ({ id, ...wp }) => wp
                        ),
                    };
                }

                return cleanObject({
                    ...rest,
                    motionpath: {
                        interpolation: motionPath.interpolation,
                        positionwaypoints: motionPath.waypoints.map(
                            ({ id, ...wp }) => wp
                        ),
                    },
                    ...backendRotation,
                    component: backendComponent,
                });
            });

            const backendPulses = pulses.map((p) => ({
                name: p.name,
                type: p.pulseType === 'cw' ? 'continuous' : 'file',
                power: p.power,
                carrier: p.carrier,
                filename: p.pulseType === 'file' ? p.filename : undefined,
            }));

            const backendTimings = timings.map((t) => {
                const { id, type, ...rest } = t;
                const timingObj = {
                    ...rest,
                    synconpulse: false,
                    freq_offset: t.freqOffset,
                    random_freq_offset_stdev: t.randomFreqOffsetStdev,
                    phase_offset: t.phaseOffset,
                    random_phase_offset_stdev: t.randomPhaseOffsetStdev,
                    noise_entries: t.noiseEntries.map(
                        ({ id, ...rest }) => rest
                    ),
                };
                // Remove noise_entries array if it's empty
                if (timingObj.noise_entries?.length === 0) {
                    delete (timingObj as any).noise_entries;
                }
                return timingObj;
            });

            const backendAntennas = antennas.map(
                ({ id, type, ...rest }) => rest
            );

            const gp_params: any = { ...globalParameters };
            // Map names back to backend convention for sync
            gp_params.starttime = gp_params.start;
            gp_params.endtime = gp_params.end;
            gp_params.randomseed = gp_params.random_seed;
            gp_params.oversample = gp_params.oversample_ratio;
            gp_params.coordinatesystem = gp_params.coordinateSystem;

            // Remove frontend-specific fields before sending
            delete gp_params.id;
            delete gp_params.type;
            delete gp_params.start;
            delete gp_params.end;
            delete gp_params.random_seed;
            delete gp_params.oversample_ratio;
            delete gp_params.coordinateSystem;
            delete gp_params.simulation_name;

            const scenarioJson = {
                simulation: {
                    name: globalParameters.simulation_name,
                    parameters: cleanObject(gp_params),
                    pulses: cleanObject(backendPulses),
                    timings: cleanObject(backendTimings),
                    antennas: cleanObject(backendAntennas),
                    platforms: backendPlatforms,
                },
            };

            try {
                const jsonPayload = JSON.stringify(scenarioJson, null, 2);
                await invoke('update_scenario_from_json', {
                    json: jsonPayload,
                });
                console.log('Successfully synced state to backend.');
            } catch (error) {
                console.error('Failed to sync state to backend:', error);
                throw error;
            }
        },

        fetchFromBackend: async () => {
            try {
                const jsonState = await invoke<string>('get_scenario_as_json');
                const scenarioData = JSON.parse(jsonState);
                get().loadScenario(scenarioData);
            } catch (error) {
                console.error('Failed to fetch state from backend:', error);
                throw error;
            }
        },
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
