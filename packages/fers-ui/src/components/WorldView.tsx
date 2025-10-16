// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useMemo } from 'react';
import { MapControls, Environment } from '@react-three/drei';
import { Vector3 } from 'three';
import { useScenarioStore, Platform } from '@/stores/scenarioStore';

/**
 * Represents a single platform in the 3D world as a sphere.
 * @param {object} props - The component props.
 * @param {Platform} props.platform - The platform data from the store.
 */
function PlatformSphere({ platform }: { platform: Platform }) {
    const position = useMemo(() => {
        // Use the first waypoint for the platform's initial position.
        // If no waypoints exist, default to the origin.
        const waypoint = platform.motionPath.waypoints[0];
        if (!waypoint) {
            return new Vector3(0, 0, 0);
        }

        // The FERS coordinate system is ENU (East-North-Up).
        // Map to Three.js's coordinate system (Y-up):
        // ENU X (East) -> Three.js X
        // ENU Altitude (Up) -> Three.js Y
        // ENU Y (North) -> Three.js -Z
        return new Vector3(waypoint.x, waypoint.altitude, -waypoint.y);
    }, [platform.motionPath.waypoints]);

    return (
        <mesh position={position} castShadow>
            <sphereGeometry args={[0.5, 32, 32]} />
            <meshStandardMaterial
                color="#90caf9"
                roughness={0.3}
                metalness={0.5}
            />
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
            {/* Controls - More suitable for map-like navigation */}
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
                shadow-camera-far={200}
                shadow-camera-left={-100}
                shadow-camera-right={100}
                shadow-camera-top={100}
                shadow-camera-bottom={-100}
            />
            {/* Environment for realistic reflections and ambient light */}
            <Environment preset="city" />

            {/* Scenery */}
            <gridHelper args={[200, 200]} />
            <mesh
                rotation={[-Math.PI / 2, 0, 0]}
                position={[0, -0.01, 0]}
                receiveShadow
            >
                <planeGeometry args={[200, 200]} />
                <shadowMaterial opacity={0.3} />
            </mesh>

            {/* Objects */}
            {platforms.map((platform) => (
                <PlatformSphere key={platform.id} platform={platform} />
            ))}
        </>
    );
}
