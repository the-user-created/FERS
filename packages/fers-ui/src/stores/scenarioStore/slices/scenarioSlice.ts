// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { StateCreator } from 'zustand';
import { v4 as uuidv4 } from 'uuid';
import {
    ScenarioStore,
    ScenarioActions,
    GlobalParameters,
    Pulse,
    Timing,
    Antenna,
    Platform,
    MotionPath,
    FixedRotation,
    RotationPath,
    PlatformComponent,
    ScenarioData,
} from '../types';
import { createDefaultPlatform, defaultGlobalParameters } from '../defaults';
import { setPropertyByPath } from '../utils';
import { ScenarioDataSchema } from '../../scenarioSchema';

// Define interfaces for the expected backend data structure.
interface BackendObjectWithName {
    name: string;
    [key: string]: unknown;
}

interface BackendPlatformComponentData {
    name: string;
    antenna?: string;
    timing?: string;
    pulse?: string;
    noise_temp?: number | null;
    nodirect?: boolean;
    nopropagationloss?: boolean;
    type?: 'cw' | 'pulsed';
    window_skip?: number;
    window_length?: number;
    prf?: number;
    rcs?: { type: 'isotropic' | 'file'; value?: number; filename?: string };
    model?: { type: 'constant' | 'chisquare' | 'gamma'; k?: number };
}

// Backend waypoint types (frontend type minus 'id')
interface BackendPositionWaypoint {
    x: number;
    y: number;
    altitude: number;
    time: number;
}

interface BackendRotationWaypoint {
    azimuth: number;
    elevation: number;
    time: number;
}

interface BackendPlatform {
    name: string;
    motionpath?: {
        interpolation: 'static' | 'linear' | 'cubic';
        positionwaypoints?: BackendPositionWaypoint[];
    };
    fixedrotation?: {
        startazimuth: number;
        startelevation: number;
        azimuthrate: number;
        elevationrate: number;
    };
    rotationpath?: {
        interpolation: 'static' | 'linear' | 'cubic';
        rotationwaypoints?: BackendRotationWaypoint[];
    };
    component?: Record<string, BackendPlatformComponentData>;
}

export const createScenarioSlice: StateCreator<
    ScenarioStore,
    [['zustand/immer', never]],
    [],
    ScenarioActions
> = (set) => ({
    selectItem: (itemId) => set({ selectedItemId: itemId }),
    updateItem: (itemId, propertyPath, value) =>
        set((state) => {
            if (itemId === 'global-parameters') {
                setPropertyByPath(state.globalParameters, propertyPath, value);
                state.isDirty = true;
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
                    state.isDirty = true;
                    return;
                }
            }
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
                    state.isDirty = true;
                    return;
                }
            }
        }),
    resetScenario: () =>
        set({
            globalParameters: defaultGlobalParameters,
            pulses: [],
            timings: [],
            antennas: [],
            platforms: [],
            selectedItemId: null,
            isDirty: false,
        }),
    loadScenario: (backendData: unknown) => {
        try {
            if (typeof backendData !== 'object' || backendData === null) {
                console.error('Invalid backend data format: not an object.');
                return;
            }

            const data =
                'simulation' in backendData &&
                typeof (backendData as { simulation: unknown }).simulation ===
                    'object' &&
                (backendData as { simulation: unknown }).simulation !== null
                    ? ((backendData as { simulation: object })
                          .simulation as Record<string, unknown>)
                    : (backendData as Record<string, unknown>);

            const nameToIdMap = new Map<string, string>();
            const assignId = <T extends object>(item: T) => ({
                ...item,
                id: uuidv4(),
            });

            // 1. Parameters
            const params = (data.parameters as Record<string, unknown>) || {};
            const globalParameters: GlobalParameters = {
                id: 'global-parameters',
                type: 'GlobalParameters',
                simulation_name: (data.name as string) || 'FERS Simulation',
                start: (params.starttime as number) ?? 0.0,
                end: (params.endtime as number) ?? 10.0,
                rate: (params.rate as number) ?? 10000.0,
                simSamplingRate: (params.simSamplingRate as number) ?? null,
                c: (params.c as number) ?? 299792458.0,
                random_seed: (params.randomseed as number) ?? null,
                adc_bits: (params.adc_bits as number) ?? 12,
                oversample_ratio: (params.oversample as number) ?? 1,
                origin: {
                    latitude:
                        ((params.origin as Record<string, number>)
                            ?.latitude as number) ?? -33.957652,
                    longitude:
                        ((params.origin as Record<string, number>)
                            ?.longitude as number) ?? 18.4611991,
                    altitude:
                        ((params.origin as Record<string, number>)
                            ?.altitude as number) ?? 111.01,
                },
                coordinateSystem: {
                    frame:
                        ((params.coordinatesystem as Record<string, string>)
                            ?.frame as GlobalParameters['coordinateSystem']['frame']) ??
                        'ENU',
                    zone: (params.coordinatesystem as Record<string, number>)
                        ?.zone,
                    hemisphere: (
                        params.coordinatesystem as Record<string, 'N' | 'S'>
                    )?.hemisphere,
                },
                export: {
                    xml:
                        ((params.export as Record<string, boolean>)
                            ?.xml as boolean) ?? false,
                    csv:
                        ((params.export as Record<string, boolean>)
                            ?.csv as boolean) ?? false,
                    binary:
                        ((params.export as Record<string, boolean>)
                            ?.binary as boolean) ?? true,
                },
            };

            // 2. Assets (and build name-to-id map)
            const pulses: Pulse[] = (
                (data.pulses as BackendObjectWithName[]) || []
            ).map((p) => {
                const pulse = {
                    ...assignId(p),
                    type: 'Pulse' as const,
                    pulseType:
                        p.type === 'continuous'
                            ? ('cw' as const)
                            : ('file' as const),
                    filename: (p.filename as string) ?? '',
                };
                nameToIdMap.set(pulse.name, pulse.id);
                return pulse as Pulse;
            });

            const timings: Timing[] = (
                (data.timings as BackendObjectWithName[]) || []
            ).map((t) => {
                const timing = {
                    ...assignId(t),
                    type: 'Timing' as const,
                    freqOffset: (t.freq_offset as number) ?? null,
                    randomFreqOffsetStdev:
                        (t.random_freq_offset_stdev as number) ?? null,
                    phaseOffset: (t.phase_offset as number) ?? null,
                    randomPhaseOffsetStdev:
                        (t.random_phase_offset_stdev as number) ?? null,
                    noiseEntries: Array.isArray(t.noise_entries)
                        ? t.noise_entries.map((item) =>
                              assignId(item as object)
                          )
                        : [],
                };
                nameToIdMap.set(timing.name, timing.id);
                return timing as Timing;
            });

            const antennas: Antenna[] = (
                (data.antennas as BackendObjectWithName[]) || []
            ).map((a) => {
                const antenna = {
                    ...assignId(a),
                    type: 'Antenna' as const,
                };
                nameToIdMap.set(antenna.name, antenna.id);
                return antenna as Antenna;
            });

            // 3. Platforms
            const platforms: Platform[] = (
                (data.platforms as BackendPlatform[]) || []
            ).map((p): Platform => {
                const motionPath: MotionPath = {
                    interpolation: p.motionpath?.interpolation ?? 'static',
                    waypoints: (p.motionpath?.positionwaypoints ?? []).map(
                        assignId
                    ),
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
                        interpolation: p.rotationpath.interpolation ?? 'static',
                        waypoints: (p.rotationpath.rotationwaypoints || []).map(
                            assignId
                        ),
                    };
                } else {
                    rotation = createDefaultPlatform().rotation as
                        | FixedRotation
                        | RotationPath;
                }

                let component: PlatformComponent = { type: 'none' };
                if (p.component && Object.keys(p.component).length > 0) {
                    const cType = Object.keys(p.component)[0];
                    const cData = p.component[cType];
                    const commonRadar = {
                        antennaId: nameToIdMap.get(cData.antenna ?? '') ?? null,
                        timingId: nameToIdMap.get(cData.timing ?? '') ?? null,
                    };
                    const commonReceiver = {
                        noiseTemperature: cData.noise_temp ?? null,
                        noDirectPaths: cData.nodirect ?? false,
                        noPropagationLoss: cData.nopropagationloss ?? false,
                    };

                    switch (cType) {
                        case 'monostatic':
                            component = {
                                type: 'monostatic',
                                name: cData.name,
                                radarType:
                                    cData.type === 'cw' ? 'cw' : 'pulsed',
                                window_skip: cData.window_skip ?? 0,
                                window_length: cData.window_length ?? 0,
                                prf: cData.prf ?? 1000,
                                pulseId:
                                    nameToIdMap.get(cData.pulse ?? '') ?? null,
                                ...commonRadar,
                                ...commonReceiver,
                            };
                            break;
                        case 'transmitter':
                            component = {
                                type: 'transmitter',
                                name: cData.name,
                                radarType:
                                    cData.type === 'cw' ? 'cw' : 'pulsed',
                                prf: cData.prf ?? 1000,
                                pulseId:
                                    nameToIdMap.get(cData.pulse ?? '') ?? null,
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
                                rcs_type: cData.rcs?.type ?? 'isotropic',
                                rcs_value: cData.rcs?.value,
                                rcs_filename: cData.rcs?.filename,
                                rcs_model: cData.model?.type ?? 'constant',
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
            });

            const transformedScenario: ScenarioData = {
                globalParameters,
                pulses,
                timings,
                antennas,
                platforms,
            };

            // --- Validate and Commit ---
            const result = ScenarioDataSchema.safeParse(transformedScenario);

            if (!result.success) {
                console.error(
                    'Data validation failed after loading from backend:',
                    result.error.flatten()
                );
                return;
            }

            // Update state with the validated and parsed data.
            set({
                ...result.data,
                selectedItemId: null,
                isDirty: true,
            });
        } catch (error) {
            console.error(
                'An unexpected error occurred while loading the scenario:',
                error
            );
        }
    },
});
