// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useEffect, useMemo, useState } from 'react';
import { MapControls, Environment, Html } from '@react-three/drei';
import { Vector3 } from 'three';
import { invoke } from '@tauri-apps/api/core';
import { useScenarioStore, Platform } from '@/stores/scenarioStore';
import { MotionPathLine } from './MotionPathLine';

// --- Type definitions for Tauri backend communication ---
const NUM_PATH_POINTS = 100;
type InterpolationType = 'static' | 'linear' | 'cubic';
interface InterpolatedPoint {
    x: number;
    y: number;
    z: number;
}

/**
 * Custom hook to calculate a platform's position at a given simulation time.
 * It fetches a high-resolution path from the backend once, then performs
 * client-side interpolation for smooth animation.
 * @param {Platform} platform The platform data.
 * @param {number} currentTime The current global simulation time.
 * @returns {Vector3} The interpolated position in Three.js coordinates.
 */
function useInterpolatedPosition(
    platform: Platform,
    currentTime: number
): Vector3 {
    const [pathPoints, setPathPoints] = useState<Vector3[]>([]);
    const { waypoints, interpolation } = platform.motionPath;

    useEffect(() => {
        const fetchPath = async () => {
            if (waypoints.length < 2 || interpolation === 'static') {
                const staticPoints = waypoints.map(
                    (wp) => new Vector3(wp.x, wp.altitude, -wp.y)
                );
                setPathPoints(staticPoints);
                return;
            }
            try {
                const points = await invoke<InterpolatedPoint[]>(
                    'get_interpolated_motion_path',
                    {
                        waypoints,
                        interpType: interpolation as InterpolationType,
                        numPoints: NUM_PATH_POINTS,
                    }
                );
                setPathPoints(points.map((p) => new Vector3(p.x, p.z, -p.y)));
            } catch (error) {
                console.error(
                    'Failed to fetch motion path for interpolation:',
                    error
                );
                setPathPoints([]);
            }
        };
        void fetchPath();
    }, [waypoints, interpolation]);

    return useMemo(() => {
        const firstWaypoint = waypoints[0];
        const lastWaypoint = waypoints[waypoints.length - 1];

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
    }, [currentTime, pathPoints, waypoints, interpolation]);
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
                occlude // Hide label if behind another object
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

    return (
        <>
            {/* Controls */}
            <MapControls makeDefault />

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
