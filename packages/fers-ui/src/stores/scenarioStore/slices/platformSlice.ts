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

interface InterpolatedRotationPoint {
    azimuth_deg: number;
    elevation_deg: number;
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
                        schedule: [],
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
                        schedule: [],
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
                        schedule: [],
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

        // 1. Fetch/Calculate Motion Path
        const { waypoints, interpolation } = platform.motionPath;
        let newPathPoints: Vector3[] = [];

        try {
            if (waypoints.length < 2 || interpolation === 'static') {
                // Static or single point: Calculate directly on frontend
                newPathPoints = waypoints.map(
                    (wp) => new Vector3(wp.x, wp.altitude, -wp.y)
                );
            } else {
                // Dynamic: Fetch interpolated points from Backend
                const points = await invoke<InterpolatedPoint[]>(
                    'get_interpolated_motion_path',
                    {
                        waypoints,
                        interpType: interpolation as InterpolationType,
                        numPoints: NUM_PATH_POINTS,
                    }
                );
                // Convert ENU (Backend) to Three.js coordinates
                newPathPoints = points.map((p) => new Vector3(p.x, p.z, -p.y));
            }
        } catch (error) {
            const msg = error instanceof Error ? error.message : String(error);
            console.error(
                `Failed to fetch motion path for ${platform.name}:`,
                msg
            );
            showError(`Failed to get motion path for ${platform.name}: ${msg}`);
            // Fallback to empty to prevent stale data
            newPathPoints = [];
        }

        // 2. Fetch/Calculate Rotation Path
        const { rotation } = platform;
        let newRotationPoints:
            | { azimuth: number; elevation: number }[]
            | undefined = undefined;

        if (rotation.type === 'path') {
            const rotWaypoints = rotation.waypoints;
            // Only fetch from backend if dynamic. Static/Single points are handled by the real-time calculator.
            if (
                rotWaypoints.length >= 2 &&
                rotation.interpolation !== 'static'
            ) {
                try {
                    const points = await invoke<InterpolatedRotationPoint[]>(
                        'get_interpolated_rotation_path',
                        {
                            waypoints: rotWaypoints,
                            interpType:
                                rotation.interpolation as InterpolationType,
                            numPoints: NUM_PATH_POINTS,
                        }
                    );
                    newRotationPoints = points.map((p) => ({
                        azimuth: p.azimuth_deg,
                        elevation: p.elevation_deg,
                    }));
                } catch (error) {
                    const msg =
                        error instanceof Error ? error.message : String(error);
                    console.error(
                        `Failed to fetch rotation path for ${platform.name}:`,
                        msg
                    );
                    // Log error but don't break the whole update; standard calc will fallback to first waypoint
                }
            }
        }

        // 3. Update Store
        set((state) => {
            const p = state.platforms.find((p) => p.id === platformId);
            if (p) {
                p.pathPoints = newPathPoints;
                p.rotationPathPoints = newRotationPoints;
            }
        });
    },
});
