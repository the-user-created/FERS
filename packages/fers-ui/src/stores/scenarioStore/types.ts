// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { z } from 'zod';
import {
    GlobalParametersSchema,
    PulseSchema,
    TimingSchema,
    AntennaSchema,
    PlatformSchema,
    ScenarioDataSchema,
    NoiseEntrySchema,
    PositionWaypointSchema,
    RotationWaypointSchema,
    PlatformComponentSchema,
    FixedRotationSchema,
    RotationPathSchema,
    MotionPathSchema,
} from '../scenarioSchema';

// --- Zod Inferred Types ---
export type GlobalParameters = z.infer<typeof GlobalParametersSchema>;
export type Pulse = z.infer<typeof PulseSchema>;
export type NoiseEntry = z.infer<typeof NoiseEntrySchema>;
export type Timing = z.infer<typeof TimingSchema>;
export type Antenna = z.infer<typeof AntennaSchema>;
export type PositionWaypoint = z.infer<typeof PositionWaypointSchema>;
export type MotionPath = z.infer<typeof MotionPathSchema>;
export type FixedRotation = z.infer<typeof FixedRotationSchema>;
export type RotationWaypoint = z.infer<typeof RotationWaypointSchema>;
export type RotationPath = z.infer<typeof RotationPathSchema>;
export type PlatformComponent = z.infer<typeof PlatformComponentSchema>;
export type Platform = z.infer<typeof PlatformSchema>;
export type TargetComponent = Extract<PlatformComponent, { type: 'target' }>;
export type ScenarioData = z.infer<typeof ScenarioDataSchema>;
export type ScenarioItem =
    | GlobalParameters
    | Pulse
    | Timing
    | Antenna
    | Platform;

// --- Store Shape ---
export type ScenarioState = ScenarioData & {
    selectedItemId: string | null;
    isDirty: boolean;
};

// --- Action Slice Types ---
export type AssetActions = {
    addPulse: () => void;
    addTiming: () => void;
    addAntenna: () => void;
    addNoiseEntry: (timingId: string) => void;
    removeNoiseEntry: (timingId: string, entryId: string) => void;
    setAntennaPattern: (
        antennaId: string,
        newPattern: Antenna['pattern']
    ) => void;
};

export type PlatformActions = {
    addPlatform: () => void;
    addPositionWaypoint: (platformId: string) => void;
    removePositionWaypoint: (platformId: string, waypointId: string) => void;
    addRotationWaypoint: (platformId: string) => void;
    removeRotationWaypoint: (platformId: string, waypointId: string) => void;
    setPlatformComponentType: (
        platformId: string,
        componentType: PlatformComponent['type']
    ) => void;
    setPlatformRcsModel: (
        platformId: string,
        newModel: TargetComponent['rcs_model']
    ) => void;
};

export type ScenarioActions = {
    selectItem: (itemId: string | null) => void;
    updateItem: (itemId: string, propertyPath: string, value: unknown) => void;
    removeItem: (itemId: string) => void;
    loadScenario: (backendData: any) => void;
    resetScenario: () => void;
};

export type BackendActions = {
    syncBackend: () => Promise<void>;
    fetchFromBackend: () => Promise<void>;
};

// --- Full Store Type ---
export type FullScenarioActions = AssetActions &
    PlatformActions &
    ScenarioActions &
    BackendActions;

export type ScenarioStore = ScenarioState & FullScenarioActions;
