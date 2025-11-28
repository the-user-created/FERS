// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useMemo } from 'react';
import * as THREE from 'three';
import {
    Platform,
    calculateInterpolatedVelocity,
} from '@/stores/scenarioStore';
import { fersColors } from '@/theme';

interface VelocityArrowProps {
    platform: Platform;
    currentTime: number;
}

/**
 * Renders an arrow indicating the velocity vector of the platform.
 * The direction indicates movement direction, and length indicates speed.
 */
export function VelocityArrow({ platform, currentTime }: VelocityArrowProps) {
    const velocity = useMemo(
        () => calculateInterpolatedVelocity(platform, currentTime),
        [platform, currentTime]
    );

    const arrowHelper = useMemo(() => {
        const speed = velocity.length();
        // Don't render if static
        if (speed < 0.001) return null;

        const dir = velocity.clone().normalize();
        const origin = new THREE.Vector3(0, 0, 0); // Local origin of the container

        // We use the raw speed as length, but verify visibility for very small/large speeds
        // Ideally, we might want to log-scale or clamp for UI, but raw is physically accurate.
        return new THREE.ArrowHelper(
            dir,
            origin,
            speed,
            fersColors.physics.velocity
        );
    }, [velocity]);

    if (!arrowHelper) return null;

    return <primitive object={arrowHelper} />;
}
