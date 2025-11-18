// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { StateCreator } from 'zustand';
import { invoke } from '@tauri-apps/api/core';
import {
    ScenarioStore,
    BackendActions,
    TargetComponent,
    Timing,
} from '../types';
import { omit } from '@/utils/typeUtils.ts';

// Helper to strip null/undefined values from an object before sending to backend
const cleanObject = <T>(obj: T): T => {
    if (Array.isArray(obj)) {
        return obj.map(cleanObject) as unknown as T;
    }
    if (obj !== null && typeof obj === 'object') {
        const newObj = {} as Record<string, unknown>;
        for (const [key, value] of Object.entries(obj)) {
            if (value !== null && value !== undefined) {
                newObj[key] = cleanObject(value);
            }
        }
        return newObj as T;
    }
    return obj;
};

// --- Backend-specific type definitions ---
type BackendTarget = {
    name: string;
    rcs: {
        type: TargetComponent['rcs_type'];
        value?: number;
        filename?: string;
    };
    model?: {
        type: Exclude<TargetComponent['rcs_model'], 'constant'>;
        k?: number;
    };
};

export const createBackendSlice: StateCreator<
    ScenarioStore,
    [['zustand/immer', never]],
    [],
    BackendActions
> = (set, get) => ({
    syncBackend: async () => {
        const { globalParameters, waveforms, timings, antennas, platforms } =
            get();

        // Helper functions to map frontend asset IDs back to names for the backend
        const findAntennaName = (id: string | null) =>
            antennas.find((a) => a.id === id)?.name;
        const findWaveformName = (id: string | null) =>
            waveforms.find((p) => p.id === id)?.name;
        const findTimingName = (id: string | null) =>
            timings.find((t) => t.id === id)?.name;

        const backendPlatforms = platforms.map((p) => {
            const { component, motionPath, rotation, ...rest } = p;

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
                                antenna: findAntennaName(component.antennaId),
                                waveform: findWaveformName(
                                    component.waveformId
                                ),
                                timing: findTimingName(component.timingId),
                                noise_temp: component.noiseTemperature,
                                nodirect: component.noDirectPaths,
                                nopropagationloss: component.noPropagationLoss,
                            },
                        };
                        break;
                    case 'transmitter':
                        backendComponent = {
                            transmitter: {
                                name: component.name,
                                type: component.radarType,
                                prf: component.prf,
                                antenna: findAntennaName(component.antennaId),
                                waveform: findWaveformName(
                                    component.waveformId
                                ),
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
                                antenna: findAntennaName(component.antennaId),
                                timing: findTimingName(component.timingId),
                                noise_temp: component.noiseTemperature,
                                nodirect: component.noDirectPaths,
                                nopropagationloss: component.noPropagationLoss,
                            },
                        };
                        break;
                    case 'target':
                        {
                            const targetObj: BackendTarget = {
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

            const backendRotation: Record<string, unknown> = {};
            if (rotation.type === 'fixed') {
                const r = omit(rotation, 'type');
                backendRotation.fixedrotation = {
                    interpolation: 'constant',
                    startazimuth: r.startAzimuth,
                    startelevation: r.startElevation,
                    azimuthrate: r.azimuthRate,
                    elevationrate: r.elevationRate,
                };
            } else {
                const r = omit(rotation, 'type');
                backendRotation.rotationpath = {
                    interpolation: r.interpolation,
                    rotationwaypoints: r.waypoints.map((wp) => omit(wp, 'id')),
                };
            }

            return cleanObject({
                ...rest,
                motionpath: {
                    interpolation: motionPath.interpolation,
                    positionwaypoints: motionPath.waypoints.map((wp) =>
                        omit(wp, 'id')
                    ),
                },
                ...backendRotation,
                component: backendComponent,
            });
        });

        const backendWaveforms = waveforms.map((w) => {
            const waveformContent =
                w.waveformType === 'cw'
                    ? { cw: {} }
                    : { pulsed_from_file: { filename: w.filename } };

            return {
                name: w.name,
                power: w.power,
                carrier_frequency: w.carrier_frequency,
                ...waveformContent,
            };
        });

        const backendTimings = timings.map((t: Timing) => {
            const rest = omit(t, 'id', 'type');
            const timingObj = {
                ...rest,
                synconpulse: false,
                freq_offset: t.freqOffset,
                random_freq_offset_stdev: t.randomFreqOffsetStdev,
                phase_offset: t.phaseOffset,
                random_phase_offset_stdev: t.randomPhaseOffsetStdev,
                noise_entries: t.noiseEntries.map((entry) => omit(entry, 'id')),
            };
            // Remove noise_entries array if it's empty
            if (timingObj.noise_entries?.length === 0) {
                delete (timingObj as Partial<typeof timingObj>).noise_entries;
            }
            return timingObj;
        });

        const backendAntennas = antennas.map((a) => omit(a, 'id', 'type'));

        const {
            start,
            end,
            random_seed,
            oversample_ratio,
            coordinateSystem,
            ...gpRest
        } = globalParameters;

        const gp_params = {
            ...gpRest,
            starttime: start,
            endtime: end,
            randomseed: random_seed,
            oversample: oversample_ratio,
            coordinatesystem: coordinateSystem,
        };

        const scenarioJson = {
            simulation: {
                name: globalParameters.simulation_name,
                parameters: cleanObject(gp_params),
                waveforms: cleanObject(backendWaveforms),
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
});
