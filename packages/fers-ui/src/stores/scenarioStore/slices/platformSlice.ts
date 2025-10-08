// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { StateCreator } from 'zustand';
import { v4 as uuidv4 } from 'uuid';
import {
    ScenarioStore,
    PlatformActions,
    Platform,
    PlatformComponent,
} from '../types';
import { createDefaultPlatform } from '../defaults';

export const createPlatformSlice: StateCreator<
    ScenarioStore,
    [['zustand/immer', never]],
    [],
    PlatformActions
> = (set) => ({
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
    setPlatformComponentType: (platformId, componentType) =>
        set((state) => {
            const platform = state.platforms.find((p) => p.id === platformId);
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
            state.isDirty = true;
        }),
    setPlatformRcsModel: (platformId, newModel) =>
        set((state) => {
            const platform = state.platforms.find((p) => p.id === platformId);
            if (platform?.component.type === 'target') {
                platform.component.rcs_model = newModel;
                if (newModel === 'chisquare' || newModel === 'gamma') {
                    if (typeof platform.component.rcs_k !== 'number') {
                        platform.component.rcs_k = 1.0;
                    }
                } else {
                    delete platform.component.rcs_k;
                }
                state.isDirty = true;
            }
        }),
});
