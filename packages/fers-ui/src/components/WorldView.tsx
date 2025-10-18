// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useMemo, useState } from 'react';
import { MapControls, Environment, Html } from '@react-three/drei';
import { Vector3 } from 'three';
import { useScenarioStore, Platform } from '@/stores/scenarioStore';
import { MotionPathLine } from './MotionPathLine';

/**
 * Represents a single platform in the 3D world as a sphere with an identifying label.
 * @param {object} props - The component props.
 * @param {Platform} props.platform - The platform data from the store.
 */
function PlatformSphere({ platform }: { platform: Platform }) {
    const [isHovered, setIsHovered] = useState(false);
    const selectedItemId = useScenarioStore((state) => state.selectedItemId);
    const isSelected = platform.id === selectedItemId;

    const waypoint = platform.motionPath.waypoints[0];

    const position = useMemo(() => {
        if (!waypoint) {
            return new Vector3(0, 0, 0);
        }
        // ENU to Three.js coordinate system mapping:
        // ENU X (East) -> Three.js X
        // ENU Altitude (Up) -> Three.js Y
        // ENU Y (North) -> Three.js -Z
        return new Vector3(waypoint.x, waypoint.altitude, -waypoint.y);
    }, [waypoint]);

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
                <div>{`X: ${(waypoint?.x ?? 0).toFixed(2)}`}</div>
                <div>{`Y: ${(waypoint?.y ?? 0).toFixed(2)}`}</div>
                <div>{`Alt: ${(waypoint?.altitude ?? 0).toFixed(2)}`}</div>
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
