// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useEffect, useMemo, useState, useRef } from 'react';
import { MapControls, Environment, Html } from '@react-three/drei';
import { Vector3, Color, ShaderMaterial, Matrix4 } from 'three';
import { invoke } from '@tauri-apps/api/core';
import {
    useScenarioStore,
    Platform,
    calculateInterpolatedPosition,
} from '@/stores/scenarioStore';
import { MotionPathLine } from './MotionPathLine';
import CameraManager from './CameraManager';
import { type MapControls as MapControlsImpl } from 'three-stdlib';
import { useFrame, useThree } from '@react-three/fiber';
import { smoothstep } from 'three/src/math/MathUtils.js';


const gridVertexShader = /* glsl */ `
    varying vec3 vWorldDirection;

    // Uniforms provided by the application
    uniform mat4 uCameraMatrixWorld;
    uniform mat4 uProjectionMatrixInverse;

    void main() {
        // The 'position' attribute of our planeGeometry is in NDC-like space (-1 to 1 on x and y)
        // We use it to construct a point in Clip Space. z=1 is the far plane, w=1.
        vec4 clipSpacePos = vec4(position.xy, 1.0, 1.0);

        // Un-project from Clip Space to World Space.
        vec4 worldSpacePos = uCameraMatrixWorld * uProjectionMatrixInverse * clipSpacePos;

        // Perform perspective division to get the cartesian world coordinate.
        worldSpacePos.xyz /= worldSpacePos.w;
        
        // Calculate the direction vector from the camera to this world position.
        // cameraPosition is a built-in uniform in Three.js holding the camera's world position.
        vWorldDirection = worldSpacePos.xyz - cameraPosition;
        
        // Position the vertex in clip space to form a full-screen quad.
        gl_Position = clipSpacePos;
    }
`;

const gridFragmentShader = /* glsl */ `
    varying vec3 vWorldDirection;

    uniform float uMajorGridSpacing;
    uniform float uMinorGridSpacing;
    uniform float uFadeFactor;
    uniform float uFadeDistance;
    uniform vec3 uGridColor;

    // Calculates the opacity of a grid line based on the world coordinate,
    // line spacing, and desired line thickness. Uses fwidth for anti-aliasing.
    float getLineAlpha(float coord, float spacing, float thickness) {
        // Calculate the world-space width of a single pixel
        float pixelWidth = fwidth(coord);
        float lineHalfWidth = thickness / 2.0;
        
        // Calculate the world-space distance to the nearest grid line
        float distToLine = min(mod(coord, spacing), spacing - mod(coord, spacing));
        
        // Create a smooth gradient over the width of one pixel for anti-aliasing
        return 1.0 - smoothstep(
            lineHalfWidth - pixelWidth,
            lineHalfWidth + pixelWidth,
            distToLine
        );
    }

    void main() {
        vec3 rayDir = normalize(vWorldDirection);
        
        // Discard pixels for rays that are almost parallel to the grid plane.
        // This avoids visual artifacts at the horizon and division by near-zero.
        if (abs(rayDir.y) < 1e-6) {
            discard;
        }

        // Calculate the intersection point of the view ray with the Y=0 plane.
        // This works correctly whether the camera is above or below the plane.
        float t = -cameraPosition.y / rayDir.y;

        // Discard pixels that are behind the camera. This correctly handles
        // cases where the camera is looking away from the plane.
        if (t < 0.0) {
            discard;
        }

        vec3 worldPosition = cameraPosition + rayDir * t;
        
        // Calculate the distance from the camera for horizon fading
        float distFromCam = distance(cameraPosition, worldPosition);
        float horizonFade = 1.0 - smoothstep(uFadeDistance * 0.9, uFadeDistance, distFromCam);

        // --- Grid Line Calculation ---
        const float majorThickness = 0.05;
        const float minorThickness = 0.025;

        float majorAlpha = max(
            getLineAlpha(worldPosition.x, uMajorGridSpacing, majorThickness), 
            getLineAlpha(worldPosition.z, uMajorGridSpacing, majorThickness)
        );
        
        float minorAlpha = max(
            getLineAlpha(worldPosition.x, uMinorGridSpacing, minorThickness),
            getLineAlpha(worldPosition.z, uMinorGridSpacing, minorThickness)
        );
        
        // Combine major lines with faded minor lines
        float gridAlpha = max(majorAlpha, minorAlpha * uFadeFactor);
        
        // Apply horizon fade to the final alpha
        vec4 finalColor = vec4(uGridColor, gridAlpha * horizonFade);
        
        // Discard fully transparent pixels for a minor performance gain
        if (finalColor.a < 0.001) {
            discard;
        }
        
        gl_FragColor = finalColor;
    }
`;

/**
 * Renders a high-performance, adaptive, infinite grid on the Y=0 plane using a
 * screen-space shader.
 */
function InfiniteGrid() {
    const materialRef = useRef<ShaderMaterial>(null);
    const { camera } = useThree();

    const gridMaterial = useMemo(
        () =>
            new ShaderMaterial({
                uniforms: {
                    uMajorGridSpacing: { value: 100.0 },
                    uMinorGridSpacing: { value: 10.0 },
                    uFadeFactor: { value: 1.0 },
                    uFadeDistance: { value: 50000.0 },
                    uGridColor: { value: new Color('#888888') },
                    uCameraMatrixWorld: { value: new Matrix4() },
                    uProjectionMatrixInverse: { value: new Matrix4() },
                },
                vertexShader: gridVertexShader,
                fragmentShader: gridFragmentShader,
                transparent: true,
                depthWrite: false, // Essential for transparency to work correctly
                lights: false, // Prevents Three.js from injecting light uniforms/code
            }),
        []
    );

    useFrame(() => {
        if (!materialRef.current) return;

        // Update camera matrix uniforms for the shader
        materialRef.current.uniforms.uCameraMatrixWorld.value.copy(
            camera.matrixWorld
        );
        materialRef.current.uniforms.uProjectionMatrixInverse.value.copy(
            camera.projectionMatrixInverse
        );

        // Calculate LOD based on a logarithmic scale of the camera's distance
        // from the origin. This is more robust for grazing angles.
        const dist = camera.position.length();
        const logDist = Math.log10(Math.max(dist, 1.0));
        const logDistFloor = Math.floor(logDist);

        const majorSpacing = 10 ** logDistFloor;
        const minorSpacing = majorSpacing / 10.0;

        // Calculate the fade factor for minor lines. They fade out as the camera
        // approaches the next major LOD level.
        const t = logDist - logDistFloor; // Progress to the next LOD (0.0 to 1.0)
        const fadeFactor = 1.0 - smoothstep(0.5, 0.9, t);

        // Update shader uniforms
        materialRef.current.uniforms.uMajorGridSpacing.value = majorSpacing;
        materialRef.current.uniforms.uMinorGridSpacing.value = minorSpacing;
        materialRef.current.uniforms.uFadeFactor.value = fadeFactor;
        // Adjust fade distance based on current grid scale for a consistent look
        materialRef.current.uniforms.uFadeDistance.value = majorSpacing * 500;
    });

    return (
        // The mesh is a simple 2x2 quad that will be stretched to fill the screen
        // by the vertex shader. It's rendered first.
        <mesh renderOrder={-1000}>
            <planeGeometry args={[2, 2]} />
            <shaderMaterial ref={materialRef} args={[gridMaterial]} />
        </mesh>
    );
}

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
            <InfiniteGrid />
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
