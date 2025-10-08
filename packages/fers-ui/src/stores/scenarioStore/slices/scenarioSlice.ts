// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { StateCreator } from 'zustand';
import { ScenarioStore, ScenarioActions } from '../types';
import { createDefaultPlatform, defaultGlobalParameters } from '../defaults';
import { setPropertyByPath } from '../utils';
import { v4 as uuidv4 } from 'uuid';
import { ScenarioDataSchema } from '../../scenarioSchema';
import {
    GlobalParameters,
    MotionPath,
    FixedRotation,
    RotationPath,
    PlatformComponent,
    Platform,
    Pulse,
    Timing,
    Antenna,
    ScenarioData,
} from '../types';

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
                    randomFreqOffsetStdev: t.random_freq_offset_stdev ?? null,
                    phaseOffset: t.phase_offset ?? null,
                    randomPhaseOffsetStdev: t.random_phase_offset_stdev ?? null,
                    noiseEntries: (t.noise_entries || []).map(assignId),
                };
                nameToIdMap.set(timing.name, timing.id);
                return timing;
            });

            const antennas: Antenna[] = (data.antennas || []).map((a: any) => {
                const antenna: any = {
                    ...assignId(a),
                    type: 'Antenna',
                };
                nameToIdMap.set(antenna.name, antenna.id);
                return antenna;
            });

            // 3. Platforms
            const platforms: Platform[] = (data.platforms || []).map(
                (p: any): Platform => {
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
                    if (p.component && Object.keys(p.component).length > 0) {
                        const cType = Object.keys(p.component)[0];
                        const cData = p.component[cType];
                        const commonRadar = {
                            antennaId: nameToIdMap.get(cData.antenna) ?? null,
                            timingId: nameToIdMap.get(cData.timing) ?? null,
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
                                        nameToIdMap.get(cData.pulse) ?? null,
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
                                        nameToIdMap.get(cData.pulse) ?? null,
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
