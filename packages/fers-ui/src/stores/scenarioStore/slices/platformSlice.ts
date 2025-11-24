// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { StateCreator } from 'zustand';
import { v4 as uuidv4 } from 'uuid';
import { invoke } from '@tauri-apps/api/core';
import { Vector3 } from 'three';
import {
    ScenarioStore,
    PlatformActions,
    Platform,
    PlatformComponent,
} from '../types';
import { createDefaultPlatform } from '../defaults';

const NUM_PATH_POINTS = 100;
type InterpolationType = 'static' | 'linear' | 'cubic';
interface InterpolatedPoint {
    x: number;
    y: number;
    z: number;
}

export const createPlatformSlice: StateCreator<
    ScenarioStore,
    [['zustand/immer', never]],
    [],
    PlatformActions
> = (set, get) => ({
    addPlatform: () =>
        set((state) => {
            const id = uuidv4();
            const newName = `Platform ${state.platforms.length + 1}`;
            const newPlatform: Platform = {
                ...createDefaultPlatform(),
                id,
                name: newName,
            };
            // Defaults to empty components list
            state.platforms.push(newPlatform);
            state.isDirty = true;
        }),
    addPositionWaypoint: (platformId) =>
        set((state) => {
            const platform = state.platforms.find((p) => p.id === platformId);
            if (platform) {
                platform.motionPath.waypoints.push({
                    id: uuidv4(),
                    x: 0,
                    y: 0,
                    altitude: 0,
                    time: 0,
                });
                state.isDirty = true;
            }
        }),
    removePositionWaypoint: (platformId, waypointId) =>
        set((state) => {
            const platform = state.platforms.find((p) => p.id === platformId);
            if (platform && platform.motionPath.waypoints.length > 1) {
                const index = platform.motionPath.waypoints.findIndex(
                    (wp) => wp.id === waypointId
                );
                if (index > -1) {
                    platform.motionPath.waypoints.splice(index, 1);
                    state.isDirty = true;
                }
            }
        }),
    addRotationWaypoint: (platformId) =>
        set((state) => {
            const platform = state.platforms.find((p) => p.id === platformId);
            if (platform?.rotation.type === 'path') {
                platform.rotation.waypoints.push({
                    id: uuidv4(),
                    azimuth: 0,
                    elevation: 0,
                    time: 0,
                });
                state.isDirty = true;
            }
        }),
    removeRotationWaypoint: (platformId, waypointId) =>
        set((state) => {
            const platform = state.platforms.find((p) => p.id === platformId);
            if (
                platform?.rotation.type === 'path' &&
                platform.rotation.waypoints.length > 1
            ) {
                const index = platform.rotation.waypoints.findIndex(
                    (wp) => wp.id === waypointId
                );
                if (index > -1) {
                    platform.rotation.waypoints.splice(index, 1);
                    state.isDirty = true;
                }
            }
        }),
    addPlatformComponent: (platformId, componentType) =>
        set((state) => {
            const platform = state.platforms.find((p) => p.id === platformId);
            if (!platform) return;

            const id = uuidv4();
            const name = `${platform.name} ${
                componentType.charAt(0).toUpperCase() + componentType.slice(1)
            }`;
            let newComponent: PlatformComponent;

            switch (componentType) {
                case 'monostatic':
                    newComponent = {
                        id,
                        type: 'monostatic',
                        name,
                        radarType: 'pulsed',
                        window_skip: 0,
                        window_length: 1e-5,
                        prf: 1000,
                        antennaId: null,
                        waveformId: null,
                        timingId: null,
                        noiseTemperature: 290,
                        noDirectPaths: false,
                        noPropagationLoss: false,
                    };
                    break;
                case 'transmitter':
                    newComponent = {
                        id,
                        type: 'transmitter',
                        name,
                        radarType: 'pulsed',
                        prf: 1000,
                        antennaId: null,
                        waveformId: null,
                        timingId: null,
                    };
                    break;
                case 'receiver':
                    newComponent = {
                        id,
                        type: 'receiver',
                        name,
                        radarType: 'pulsed',
                        window_skip: 0,
                        window_length: 1e-5,
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
                        id,
                        type: 'target',
                        name,
                        rcs_type: 'isotropic',
                        rcs_value: 1,
                        rcs_model: 'constant',
                    };
                    break;
                default:
                    return;
            }
            platform.components.push(newComponent);
            state.isDirty = true;
        }),
    removePlatformComponent: (platformId, componentId) =>
        set((state) => {
            const platform = state.platforms.find((p) => p.id === platformId);
            if (platform) {
                const index = platform.components.findIndex(
                    (c) => c.id === componentId
                );
                if (index > -1) {
                    platform.components.splice(index, 1);
                    state.isDirty = true;
                }
            }
        }),
    setPlatformRcsModel: (platformId, componentId, newModel) =>
        set((state) => {
            const platform = state.platforms.find((p) => p.id === platformId);
            const component = platform?.components.find(
                (c) => c.id === componentId
            );
            if (component?.type === 'target') {
                component.rcs_model = newModel;
                if (newModel === 'chisquare' || newModel === 'gamma') {
                    if (typeof component.rcs_k !== 'number') {
                        component.rcs_k = 1.0;
                    }
                } else {
                    delete component.rcs_k;
                }
                state.isDirty = true;
            }
        }),
    fetchPlatformPath: async (platformId) => {
        const { platforms, showError } = get();
        const platform = platforms.find((p) => p.id === platformId);

        if (!platform) return;

        const { waypoints, interpolation } = platform.motionPath;

        if (waypoints.length < 2 || interpolation === 'static') {
            const staticPoints = waypoints.map(
                (wp) => new Vector3(wp.x, wp.altitude, -wp.y)
            );
            set((state) => {
                const p = state.platforms.find((p) => p.id === platformId);
                if (p) p.pathPoints = staticPoints;
            });
            return;
        }

        try {
            const points = await invoke<InterpolatedPoint[]>(
                'get_interpolated_motion_path',
                {
                    waypoints,
                    interpType: interpolation as InterpolationType,
                    numPoints: NUM_PATH_POINTS,
                }
            );
            const vectors = points.map((p) => new Vector3(p.x, p.z, -p.y));
            set((state) => {
                const p = state.platforms.find((p) => p.id === platformId);
                if (p) p.pathPoints = vectors;
            });
        } catch (error) {
            const msg = error instanceof Error ? error.message : String(error);
            console.error(
                `Failed to fetch motion path for ${platform.name}:`,
                msg
            );
            showError(`Failed to get path for ${platform.name}: ${msg}`);
            set((state) => {
                const p = state.platforms.find((p) => p.id === platformId);
                if (p) p.pathPoints = [];
            });
        }
    },
});
