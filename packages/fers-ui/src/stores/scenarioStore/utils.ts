// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { ScenarioState, ScenarioItem } from './types';

// Helper to set nested properties safely
export const setPropertyByPath = (
    obj: object,
    path: string,
    value: unknown
): void => {
    const keys = path.split('.');
    const lastKey = keys.pop();
    if (!lastKey) return;

    let current: Record<string, unknown> = obj as Record<string, unknown>;

    for (const key of keys) {
        const next = current[key];
        if (typeof next !== 'object' || next === null) {
            // Path does not exist, so we cannot set the value.
            return;
        }
        current = next as Record<string, unknown>;
    }

    current[lastKey] = value;
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
