// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { create } from 'zustand';
import { immer } from 'zustand/middleware/immer';
import { ScenarioStore } from './types';
import { defaultGlobalParameters } from './defaults';
import { createAssetSlice } from './slices/assetSlice';
import { createBackendSlice } from './slices/backendSlice';
import { createPlatformSlice } from './slices/platformSlice';
import { createScenarioSlice } from './slices/scenarioSlice';
export * from './types';
export * from './utils';

export const useScenarioStore = create<ScenarioStore>()(
    immer((set, get, store) => ({
        // Initial State
        globalParameters: defaultGlobalParameters,
        waveforms: [],
        timings: [],
        antennas: [],
        platforms: [],
        selectedItemId: null,
        isDirty: false,
        isPlaying: false,
        currentTime: 0,
        targetPlaybackDuration: null,
        isSimulating: false,
        errorSnackbar: {
            open: false,
            message: '',
        },
        viewControlAction: { type: null, timestamp: 0 },

        // Slices
        ...createAssetSlice(set, get, store),
        ...createBackendSlice(set, get, store),
        ...createPlatformSlice(set, get, store),
        ...createScenarioSlice(set, get, store),

        // Playback Actions
        togglePlayPause: () =>
            set((state) => ({ isPlaying: !state.isPlaying })),
        setCurrentTime: (time) => {
            const { start, end } = get().globalParameters;
            const newTime =
                typeof time === 'function' ? time(get().currentTime) : time;
            // Clamp time to simulation bounds
            const clampedTime = Math.max(start, Math.min(end, newTime));
            set({ currentTime: clampedTime });
        },
        setTargetPlaybackDuration: (duration) =>
            set({
                targetPlaybackDuration:
                    duration !== null && duration > 0 ? duration : null,
            }),
        setIsSimulating: (isSimulating) => set({ isSimulating }),

        frameScene: () =>
            set({
                viewControlAction: { type: 'frame', timestamp: Date.now() },
            }),
        focusOnItem: (itemId) =>
            set({
                viewControlAction: {
                    type: 'focus',
                    targetId: itemId,
                    timestamp: Date.now(),
                },
            }),
        toggleFollowItem: (itemId) => {
            const currentAction = get().viewControlAction;
            if (
                currentAction.type === 'follow' &&
                currentAction.targetId === itemId
            ) {
                set({
                    viewControlAction: { type: null, timestamp: Date.now() },
                });
            } else {
                set({
                    viewControlAction: {
                        type: 'follow',
                        targetId: itemId,
                        timestamp: Date.now(),
                    },
                });
            }
        },
        clearViewControlAction: () =>
            set((state) => {
                // Don't clear a 'follow' action, as it's persistent until toggled off.
                if (state.viewControlAction.type !== 'follow') {
                    return {
                        viewControlAction: {
                            type: null,
                            timestamp: state.viewControlAction.timestamp,
                        },
                    };
                }
                return {};
            }),

        // Error Actions
        showError: (message) => set({ errorSnackbar: { open: true, message } }),
        hideError: () =>
            set((state) => ({
                errorSnackbar: { ...state.errorSnackbar, open: false },
            })),
    }))
);
