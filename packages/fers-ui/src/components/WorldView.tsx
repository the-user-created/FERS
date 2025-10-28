// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useEffect, useMemo, useState, useRef } from 'react';
import { MapControls, Environment, Html } from '@react-three/drei';
import { Vector3 } from 'three';
import {
    useScenarioStore,
    Platform,
    calculateInterpolatedPosition,
} from '@/stores/scenarioStore';
import { MotionPathLine } from './MotionPathLine';
import CameraManager from './CameraManager';
import { type MapControls as MapControlsImpl } from 'three-stdlib';

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
        [platform, currentTime, platform.pathPoints] // Depend on pathPoints
    );
}

/**
 * Represents a single platform in the 3D world as a sphere with an identifying label.
 * @param {object} props - The component props.
 * @param {Platform} props.platform - The platform data from the store.
 */
function PlatformSphere({ platform }: { platform: Platform }) {
    const [isHovered, setIsHovered] = useState(false);
    const selectedItemId = useScenarioStore((state) => state.selectedItemId);
    const currentTime = useScenarioStore((state) => state.currentTime);
    const isSelected = platform.id === selectedItemId;

    const position = useInterpolatedPosition(platform, currentTime);

    const labelData = useMemo(
        () => ({
            x: position?.x,
            y: -position?.z, // Convert from Three.js Z back to ENU Y
            altitude: position?.y, // Convert from Three.js Y back to ENU Altitude
        }),
        [position]
    );

    return (
        <mesh
            position={position}
            castShadow
            onPointerOver={(e) => {
                e.stopPropagation();
                setIsHovered(true);
            }}
            onPointerOut={() => setIsHovered(false)}
        >
            <sphereGeometry args={[0.5, 32, 32]} />
            <meshStandardMaterial
                color={isHovered || isSelected ? '#f48fb1' : '#90caf9'}
                roughness={0.3}
                metalness={0.5}
                emissive={isSelected ? '#f48fb1' : '#000000'}
                emissiveIntensity={isSelected ? 0.25 : 0}
            />
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
                    pointerEvents: 'none', // Allow clicking/dragging "through" the label
                    transition: 'opacity 0.2s, border 0.2s',
                    opacity: isHovered || isSelected ? 1 : 0.75,
                    border: isSelected ? '2px solid #f48fb1' : 'none',
                    userSelect: 'none',
                }}
            >
                <div style={{ fontWeight: 'bold' }}>{platform.name}</div>
                <div>{`X: ${(labelData?.x ?? 0).toFixed(2)}`}</div>
                <div>{`Y: ${(labelData?.y ?? 0).toFixed(2)}`}</div>
                <div>{`Alt: ${(labelData?.altitude ?? 0).toFixed(2)}`}</div>
            </Html>
        </mesh>
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
    const controlsRef = useRef<MapControlsImpl>(null);

    // Memoize platform dependencies to avoid re-triggering the effect unnecessarily.
    const platformDeps = useMemo(
        () =>
            platforms
                .map((p) =>
                    [
                        p.id,
                        p.motionPath.interpolation,
                        p.motionPath.waypoints.length,
                    ].join()
                )
                .join(';'),
        [platforms]
    );

    useEffect(() => {
        platforms.forEach((platform) => {
            // Fetch path data for any platform that doesn't have it yet.
            // This effect re-runs if waypoints or interpolation type changes.
            if (!platform.pathPoints) {
                void fetchPlatformPath(platform.id);
            }
        });
    }, [platformDeps, fetchPlatformPath, platforms]);

    return (
        <>
            <CameraManager controlsRef={controlsRef} />

            {/* Controls */}
            <MapControls makeDefault ref={controlsRef} />

            {/* Lighting */}
            <ambientLight intensity={0.5} />
            <directionalLight
                castShadow
                position={[50, 50, 25]}
                intensity={2.5}
                shadow-mapSize-width={2048}
                shadow-mapSize-height={2048}
                shadow-camera-near={0.5}
                shadow-camera-far={500}
                shadow-camera-left={-200}
                shadow-camera-right={200}
                shadow-camera-top={200}
                shadow-camera-bottom={-200}
            />
            {/* Environment for realistic reflections and ambient light */}
            <Environment preset="city" />

            {/* Scenery */}
            <gridHelper args={[1000, 100]} />
            <mesh
                rotation={[-Math.PI / 2, 0, 0]}
                position={[0, -0.01, 0]}
                receiveShadow
            >
                <planeGeometry args={[1000, 1000]} />
                <shadowMaterial opacity={0.3} />
            </mesh>

            {/* Objects */}
            {platforms.map((platform) => (
                <group key={platform.id}>
                    <PlatformSphere platform={platform} />
                    <MotionPathLine platform={platform} />
                </group>
            ))}
        </>
    );
}
