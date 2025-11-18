// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { z } from 'zod';
import { Vector3 } from 'three';
import {
    GlobalParametersSchema,
    WaveformSchema,
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
export type Waveform = z.infer<typeof WaveformSchema>;
export type NoiseEntry = z.infer<typeof NoiseEntrySchema>;
export type Timing = z.infer<typeof TimingSchema>;
export type Antenna = z.infer<typeof AntennaSchema>;
export type PositionWaypoint = z.infer<typeof PositionWaypointSchema>;
export type MotionPath = z.infer<typeof MotionPathSchema>;
export type FixedRotation = z.infer<typeof FixedRotationSchema>;
export type RotationWaypoint = z.infer<typeof RotationWaypointSchema>;
export type RotationPath = z.infer<typeof RotationPathSchema>;
export type PlatformComponent = z.infer<typeof PlatformComponentSchema>;
export type Platform = z.infer<typeof PlatformSchema> & {
    pathPoints?: Vector3[];
};
export type TargetComponent = Extract<PlatformComponent, { type: 'target' }>;

export type ScenarioData = Omit<
    z.infer<typeof ScenarioDataSchema>,
    'platforms'
> & {
    platforms: Platform[];
};

export type ScenarioItem =
    | GlobalParameters
    | Waveform
    | Timing
    | Antenna
    | Platform;

// --- Store Shape ---
export type ViewControlAction = {
    type: 'frame' | 'focus' | 'follow' | null;
    targetId?: string;
    timestamp: number;
};

export type ScenarioState = ScenarioData & {
    selectedItemId: string | null;
    isDirty: boolean;
    isPlaying: boolean;
    currentTime: number;
    targetPlaybackDuration: number | null;
    isSimulating: boolean;
    errorSnackbar: {
        open: boolean;
        message: string;
    };
    viewControlAction: ViewControlAction;
};

// --- Action Slice Types ---
export type AssetActions = {
    addWaveform: () => void;
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
    fetchPlatformPath: (platformId: string) => Promise<void>;
};

export type ScenarioActions = {
    selectItem: (itemId: string | null) => void;
    updateItem: (itemId: string, propertyPath: string, value: unknown) => void;
    removeItem: (itemId: string) => void;
    loadScenario: (backendData: unknown) => void;
    resetScenario: () => void;
};

export type BackendActions = {
    syncBackend: () => Promise<void>;
    fetchFromBackend: () => Promise<void>;
};

export type PlaybackActions = {
    togglePlayPause: () => void;
    setCurrentTime: (time: number | ((prevTime: number) => number)) => void;
    setTargetPlaybackDuration: (duration: number | null) => void;
    setIsSimulating: (isSimulating: boolean) => void;
};

export type ErrorActions = {
    showError: (message: string) => void;
    hideError: () => void;
};

export type ViewControlActions = {
    frameScene: () => void;
    focusOnItem: (itemId: string) => void;
    toggleFollowItem: (itemId: string) => void;
    clearViewControlAction: () => void;
};

// --- Full Store Type ---
export type FullScenarioActions = AssetActions &
    PlatformActions &
    ScenarioActions &
    BackendActions &
    PlaybackActions &
    ErrorActions &
    ViewControlActions;

export type ScenarioStore = ScenarioState & FullScenarioActions;
