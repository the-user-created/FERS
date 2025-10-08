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
        pulses: [],
        timings: [],
        antennas: [],
        platforms: [],
        selectedItemId: null,
        isDirty: false,

        // Slices
        ...createAssetSlice(set, get, store),
        ...createBackendSlice(set, get, store),
        ...createPlatformSlice(set, get, store),
        ...createScenarioSlice(set, get, store),
    }))
);
