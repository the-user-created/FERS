// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { ScenarioState, ScenarioItem } from './types';

// Helper to set nested properties safely
export const setPropertyByPath = (
    obj: object,
    path: string,
    value: unknown
) => {
    const keys = path.split('.');
    let current: any = obj;
    for (let i = 0; i < keys.length - 1; i++) {
        const key = keys[i];
        if (current[key] === undefined || current[key] === null) return;
        current = current[key];
    }
    current[keys[keys.length - 1]] = value;
};

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
