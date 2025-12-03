// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useMemo, useState, useEffect, useRef } from 'react';
import * as THREE from 'three';
import { invoke } from '@tauri-apps/api/core';
import { useThree, useFrame } from '@react-three/fiber';
import { useScenarioStore } from '@/stores/scenarioStore';
import { useShallow } from 'zustand/react/shallow';

const AZIMUTH_SEGMENTS = 64; // Resolution for azimuth sampling
const ELEVATION_SEGMENTS = 32; // Resolution for elevation sampling
const BASE_MESH_RADIUS = 3; // Base visual scaling factor for the main lobe.

// TODO: when multiple meshes are present on a single platform, some z-fighting occurs

interface AntennaPatternData {
    gains: number[];
    az_count: number;
    el_count: number;
    max_gain: number;
}

interface AntennaPatternMeshProps {
    antennaId: string;
}

/**
 * Creates and renders a 3D heatmap mesh from real antenna pattern data
 * fetched from the `libfers` backend.
 *
 * @returns A Three.js mesh component representing the antenna gain pattern.
 */
export function AntennaPatternMesh({ antennaId }: AntennaPatternMeshProps) {
    const [patternData, setPatternData] = useState<AntennaPatternData | null>(
        null
    );

    // Select both the antenna object and the backend version to react to updates.
    // useShallow ensures the component only re-renders when these specific values change.
    const { antenna, backendVersion } = useScenarioStore(
        useShallow((state) => ({
            antenna: state.antennas.find((a) => a.id === antennaId),
            backendVersion: state.backendVersion,
        }))
    );

    const antennaName = antenna?.name;
    const userScale = antenna?.meshScale ?? 1.0;

    const groupRef = useRef<THREE.Group>(null!);
    const { camera } = useThree();

    useEffect(() => {
        let isCancelled = false;

        const fetchPattern = async () => {
            // If there is no valid antenna or name when this async function runs,
            // clear any existing pattern data and exit.
            if (!antenna || !antennaName) {
                if (!isCancelled) {
                    setPatternData(null);
                }
                return;
            }

            try {
                const data = await invoke<AntennaPatternData>(
                    'get_antenna_pattern',
                    {
                        antennaName: antennaName,
                        azSamples: AZIMUTH_SEGMENTS + 1,
                        elSamples: ELEVATION_SEGMENTS + 1,
                    }
                );
                if (!isCancelled) {
                    setPatternData(data);
                }
            } catch (error) {
                console.error(
                    `Failed to fetch pattern for antenna ${antennaName}:`,
                    error
                );
                // On error, also clear the data to avoid showing a stale pattern.
                if (!isCancelled) {
                    setPatternData(null);
                }
            }
        };

        void fetchPattern();

        return () => {
            isCancelled = true;
        };
        // Re-fetch when the antenna object's properties change, its name changes,
        // or after the backend has confirmed it has been updated (via backendVersion).
        // This ensures that changes made in the inspector trigger a pattern refresh.
    }, [antenna, backendVersion, antennaName]);

    useFrame(() => {
        if (!groupRef.current) return;
        // Calculate distance from camera to the mesh's world position
        const worldPosition = new THREE.Vector3();
        groupRef.current.getWorldPosition(worldPosition);
        const distance = camera.position.distanceTo(worldPosition);

        // This factor makes the mesh larger as it gets farther away,
        // creating the illusion of a constant size on screen.
        // The divisor (e.g., 50) is a reference distance that can be
        // tuned to adjust the default apparent size.
        const zoomScale = distance / 50;

        // Combine zoom scaling with the user-defined multiplier
        const finalScale = zoomScale * userScale;
        groupRef.current.scale.set(finalScale, finalScale, finalScale);
    });

    const geometry = useMemo(() => {
        if (!patternData) return new THREE.BufferGeometry();

        const geom = new THREE.BufferGeometry();
        const vertices: number[] = [];
        const colors: number[] = [];
        const { gains, az_count, el_count } = patternData;

        for (let i = 0; i < el_count; i++) {
            // Elevation from -PI/2 to PI/2
            const elevation = (i / (el_count - 1)) * Math.PI - Math.PI / 2;

            for (let j = 0; j < az_count; j++) {
                // Azimuth from -PI to PI
                const azimuth = (j / (az_count - 1)) * 2 * Math.PI - Math.PI;

                const gain = gains[i * az_count + j]; // Normalized gain [0, 1]
                const radius = gain * BASE_MESH_RADIUS;

                // Convert spherical to Cartesian for a -Z forward orientation
                const x = radius * Math.cos(elevation) * Math.sin(azimuth);
                const y = radius * Math.sin(elevation);
                const z = -radius * Math.cos(elevation) * Math.cos(azimuth);
                vertices.push(x, y, z);

                // Assign color based on gain (heatmap: blue -> green -> red)
                const color = new THREE.Color();
                // HSL: Hue from blue (0.66, low gain) to red (0.0, high gain).
                color.setHSL(0.66 * (1 - gain), 1.0, 0.5);
                colors.push(color.r, color.g, color.b);
            }
        }

        const indices: number[] = [];
        for (let i = 0; i < ELEVATION_SEGMENTS; i++) {
            for (let j = 0; j < AZIMUTH_SEGMENTS; j++) {
                const a = i * (AZIMUTH_SEGMENTS + 1) + j;
                const b = a + AZIMUTH_SEGMENTS + 1;
                const c = a + 1;
                const d = b + 1;

                indices.push(a, b, c); // First triangle
                indices.push(b, d, c); // Second triangle
            }
        }

        geom.setIndex(indices);
        geom.setAttribute(
            'position',
            new THREE.Float32BufferAttribute(vertices, 3)
        );
        geom.setAttribute('color', new THREE.Float32BufferAttribute(colors, 3));
        geom.computeVertexNormals();

        return geom;
    }, [patternData]);

    if (!patternData) return null;

    return (
        <group ref={groupRef}>
            <mesh geometry={geometry}>
                <meshStandardMaterial
                    vertexColors
                    transparent
                    opacity={0.5}
                    side={THREE.DoubleSide}
                    roughness={0.7}
                    metalness={0.1}
                />
            </mesh>
        </group>
    );
}
