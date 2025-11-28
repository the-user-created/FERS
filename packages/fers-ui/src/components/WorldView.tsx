// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useEffect, useMemo, useRef } from 'react';
import { MapControls, Environment, Html } from '@react-three/drei';
import { Vector3 } from 'three';
import {
    useScenarioStore,
    Platform,
    calculateInterpolatedPosition,
    calculateInterpolatedRotation,
} from '@/stores/scenarioStore';
import { MotionPathLine } from './MotionPathLine';
import CameraManager from './CameraManager';
import { type MapControls as MapControlsImpl } from 'three-stdlib';
import { BoresightArrow } from './BoresightArrow';
import { VelocityArrow } from './VelocityArrow';
import { AntennaPatternMesh } from './AntennaPatternMesh';
import LinkVisualizer from './LinkVisualizer';

/**
 * Custom hook to calculate a platform's position at a given simulation time.
 * It relies on the pre-fetched path data stored in the Zustand store.
 * @param {Platform} platform The platform data.
 * @param {number} currentTime The current global simulation time.
 * @returns {Vector3} The interpolated position in Three.js coordinates.
 */
function useInterpolatedPosition(
    platform: Platform,
    currentTime: number
): Vector3 {
    return useMemo(
        () => calculateInterpolatedPosition(platform, currentTime),
        [platform, currentTime]
    );
}

/**
 * Custom hook to calculate a platform's rotation at a given simulation time.
 */
function useInterpolatedRotation(platform: Platform, currentTime: number) {
    return useMemo(
        () => calculateInterpolatedRotation(platform, currentTime),
        [platform, currentTime]
    );
}

/**
 * Represents a single platform in the 3D world as a sphere with an identifying label.
 * @param {object} props - The component props.
 * @param {Platform} props.platform - The platform data from the store.
 */
function PlatformSphere({ platform }: { platform: Platform }) {
    const selectedItemId = useScenarioStore((state) => state.selectedItemId);
    const currentTime = useScenarioStore((state) => state.currentTime);
    const isSelected = platform.id === selectedItemId;

    // Visibility Toggles
    const {
        showPlatforms,
        showAxes,
        showBoresights,
        showPatterns,
        showVelocities,
    } = useScenarioStore((state) => state.visibility);

    // Hooks must run unconditionally
    const position = useInterpolatedPosition(platform, currentTime);
    const rotation = useInterpolatedRotation(platform, currentTime);

    // Find all components on this platform that have an antenna
    const antennaComponents = useMemo(() => {
        return platform.components.filter(
            (
                c
            ): c is Extract<
                typeof c,
                { type: 'monostatic' | 'transmitter' | 'receiver' }
            > =>
                (c.type === 'monostatic' ||
                    c.type === 'transmitter' ||
                    c.type === 'receiver') &&
                c.antennaId !== null
        );
    }, [platform.components]);

    // Find all target components to visualize their RCS
    const targetComponents = useMemo(() => {
        return platform.components.filter(
            (c): c is Extract<typeof c, { type: 'target' }> =>
                c.type === 'target'
        );
    }, [platform.components]);

    const labelData = useMemo(
        () => ({
            x: position?.x,
            y: -position?.z, // Convert from Three.js Z back to ENU Y
            altitude: position?.y, // Convert from Three.js Y back to ENU Altitude
        }),
        [position]
    );

    return (
        <group position={position}>
            {/* Rotation Group: Handles Body Orientation */}
            <group rotation={rotation}>
                {/* Render Platform Geometry and Axes */}
                {showPlatforms && (
                    <mesh>
                        <sphereGeometry args={[0.5, 32, 32]} />
                        <meshStandardMaterial
                            color={isSelected ? '#f48fb1' : '#90caf9'}
                            roughness={0.3}
                            metalness={0.5}
                            emissive={isSelected ? '#f48fb1' : '#000000'}
                            emissiveIntensity={isSelected ? 0.25 : 0}
                        />
                        {/* Render Body Axes: Red=X (Right), Green=Y (Up), Blue=Z (Rear). */}
                        {/* TODO: the axes should scale with zoom level for better visibility (with maybe a toggle to turn off individual platform axes) */}
                        {showAxes && <axesHelper args={[2]} />}
                    </mesh>
                )}

                {/* Visualize Isotropic Static Sphere RCS: r = sqrt(sigma / pi) */}
                {/* Only visible if platform geometry is enabled */}
                {showPlatforms &&
                    targetComponents.map((target) => {
                        {
                            /* TODO: currently only rendering constant isotropic RCS */
                        }
                        if (
                            target.rcs_type === 'isotropic' &&
                            target.rcs_value &&
                            target.rcs_value > 0
                        ) {
                            const radius = Math.sqrt(
                                target.rcs_value / Math.PI
                            );
                            return (
                                <mesh key={target.id}>
                                    <sphereGeometry args={[radius, 24, 24]} />
                                    <meshBasicMaterial
                                        color="#ff9800"
                                        wireframe
                                        transparent
                                        opacity={0.3}
                                    />
                                </mesh>
                            );
                        }
                        return null;
                    })}

                {/* Conditionally render boresight and patterns for each antenna */}
                {showBoresights && antennaComponents.length > 0 && (
                    <BoresightArrow />
                )}

                {/* Show patterns only if global toggle ON AND platform selected */}
                {showPatterns &&
                    isSelected &&
                    antennaComponents.map((comp) =>
                        comp.antennaId ? (
                            <AntennaPatternMesh
                                key={comp.id}
                                antennaId={comp.antennaId}
                            />
                        ) : null
                    )}
            </group>

            {/* Velocity Arrow: Rendered in world-aligned space (relative to position) */}
            {showVelocities && (
                <VelocityArrow platform={platform} currentTime={currentTime} />
            )}

            {/* Label: Only show if platforms are visible */}
            {showPlatforms && (
                <Html
                    position={[0, 1.2, 0]} // Position label above the sphere
                    center // Center the label on its anchor point
                    style={{
                        backgroundColor: 'rgba(20, 20, 20, 0.8)',
                        color: 'white',
                        padding: '4px 8px',
                        borderRadius: '6px',
                        fontSize: '12px',
                        whiteSpace: 'nowrap',
                        pointerEvents: 'none', // Allows camera clicks to pass through
                        userSelect: 'none', // Prevents text selection
                        transition: 'opacity 0.2s, border 0.2s',
                        opacity: isSelected ? 1 : 0.75,
                        border: isSelected ? '2px solid #f48fb1' : 'none',
                    }}
                >
                    <div style={{ fontWeight: 'bold' }}>{platform.name}</div>
                    <div>{`X: ${(labelData?.x ?? 0).toFixed(2)}`}</div>
                    <div>{`Y: ${(labelData?.y ?? 0).toFixed(2)}`}</div>
                    <div>{`Alt: ${(labelData?.altitude ?? 0).toFixed(2)}`}</div>
                </Html>
            )}
        </group>
    );
}

/**
 * WorldView represents the 3D scene where simulation elements are visualized.
 * It provides an interactive camera and renders platforms from the current scenario.
 */
export default function WorldView() {
    const platforms = useScenarioStore((state) => state.platforms);
    const fetchPlatformPath = useScenarioStore(
        (state) => state.fetchPlatformPath
    );

    // Root Level Visibility Toggles
    const { showLinks, showMotionPaths } = useScenarioStore(
        (state) => state.visibility
    );

    const controlsRef = useRef<MapControlsImpl>(null);

    // Keep a reference to platforms to access in the effect without adding it to dependencies.
    // We update this ref in a separate effect to avoid "mutation during render" errors.
    const platformsRef = useRef(platforms);

    useEffect(() => {
        platformsRef.current = platforms;
    }, [platforms]);

    // Memoize platform dependencies.
    const platformDeps = useMemo(
        () =>
            platforms
                .map((p) => {
                    const rotKey =
                        p.rotation.type === 'path'
                            ? `${p.rotation.interpolation}-${JSON.stringify(
                                  p.rotation.waypoints
                              )}`
                            : `fixed-${p.rotation.startAzimuth}-${p.rotation.startElevation}-${p.rotation.azimuthRate}-${p.rotation.elevationRate}`;

                    return [
                        p.id,
                        p.motionPath.interpolation,
                        JSON.stringify(p.motionPath.waypoints),
                        rotKey,
                    ].join('|');
                })
                .join(';'),
        [platforms]
    );

    useEffect(() => {
        // Access the platforms via ref. The dependency array is strictly controlled
        // by platformDeps, which prevents the infinite loop caused by store updates.
        platformsRef.current.forEach((platform) => {
            void fetchPlatformPath(platform.id);
        });
    }, [platformDeps, fetchPlatformPath]);

    return (
        <>
            <CameraManager controlsRef={controlsRef} />

            {/* Controls */}
            <MapControls makeDefault ref={controlsRef} />

            {/* Lighting */}
            <ambientLight intensity={0.5} />
            <directionalLight position={[50, 50, 25]} intensity={2.5} />

            {/* Environment for realistic reflections and ambient light */}
            <Environment files="/potsdamer_platz_1k.hdr" />

            {/* Physics Link Visualization - Conditional Render */}
            {showLinks && <LinkVisualizer />}

            {/* Objects */}
            {platforms.map((platform) => (
                <group key={platform.id}>
                    <PlatformSphere platform={platform} />
                    {/* Motion Path Lines - Conditional Render */}
                    {showMotionPaths && <MotionPathLine platform={platform} />}
                </group>
            ))}
        </>
    );
}
