// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { ScenarioState, ScenarioItem, Platform } from './types';
import { Vector3 } from 'three';

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

/**
 * Calculates a platform's interpolated 3D position at a specific time.
 * This function relies on the pre-fetched `pathPoints` array stored on the platform object.
 * @param {Platform} platform The platform data, including its waypoints and cached path points.
 * @param {number} currentTime The global simulation time.
 * @returns {Vector3} The interpolated position in Three.js coordinates.
 */
export function calculateInterpolatedPosition(
    platform: Platform,
    currentTime: number
): Vector3 {
    const { waypoints, interpolation } = platform.motionPath;
    const pathPoints = platform.pathPoints ?? [];

    const firstWaypoint = waypoints[0];
    if (!firstWaypoint) return new Vector3(0, 0, 0);

    const staticPosition = new Vector3(
        firstWaypoint.x ?? 0,
        firstWaypoint.altitude ?? 0,
        -(firstWaypoint.y ?? 0)
    );

    if (
        interpolation === 'static' ||
        waypoints.length < 2 ||
        pathPoints.length < 2
    ) {
        return staticPosition;
    }

    const lastWaypoint = waypoints[waypoints.length - 1];
    const pathStartTime = firstWaypoint.time;
    const pathEndTime = lastWaypoint.time;
    const pathDuration = pathEndTime - pathStartTime;

    if (pathDuration <= 0) return staticPosition;

    const timeRatio = (currentTime - pathStartTime) / pathDuration;
    const clampedRatio = Math.max(0, Math.min(1, timeRatio));

    const floatIndex = clampedRatio * (pathPoints.length - 1);
    const index1 = Math.floor(floatIndex);
    const index2 = Math.min(pathPoints.length - 1, Math.ceil(floatIndex));

    const point1 = pathPoints[index1];
    const point2 = pathPoints[index2];

    if (!point1 || !point2) return staticPosition;
    if (index1 === index2) return point1.clone();

    const interPointRatio = floatIndex - index1;
    return point1.clone().lerp(point2, interPointRatio);
}
