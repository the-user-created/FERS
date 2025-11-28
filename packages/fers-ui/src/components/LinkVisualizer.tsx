// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useState, useEffect, useMemo } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { Line, Html } from '@react-three/drei';
import * as THREE from 'three';
import { useScenarioStore } from '@/stores/scenarioStore';

interface VisualLink {
    link_type: 'monostatic' | 'illuminator' | 'scattered' | 'direct';
    quality: 'strong' | 'weak';
    start: [number, number, number];
    end: [number, number, number];
    label: string;
}

const COLORS = {
    monostatic: { strong: '#00e676', weak: '#f44336' }, // Bright Green / Red
    illuminator: '#ffeb3b', // Yellow
    scattered: '#00bcd4', // Cyan
    direct: '#9c27b0', // Purple
};

// Helper to determine color based on link type and quality
const getLinkColor = (link: VisualLink) => {
    if (link.link_type === 'monostatic') {
        return link.quality === 'strong'
            ? COLORS.monostatic.strong
            : COLORS.monostatic.weak;
    } else if (link.link_type === 'illuminator') {
        return COLORS.illuminator;
    } else if (link.link_type === 'scattered') {
        return COLORS.scattered;
    } else if (link.link_type === 'direct') {
        return COLORS.direct;
    }
    return '#ffffff';
};

// Component responsible solely for rendering the 3D line geometry
function LinkLine({ link }: { link: VisualLink }) {
    // Coordinate Transform: Physics (ENU) -> Visual (Three.js)
    const points = useMemo(() => {
        const start = new THREE.Vector3(
            link.start[0],
            link.start[2],
            -link.start[1]
        );
        const end = new THREE.Vector3(link.end[0], link.end[2], -link.end[1]);
        return [start, end] as [THREE.Vector3, THREE.Vector3];
    }, [link.start, link.end]);

    const color = getLinkColor(link);
    let dashed = false;

    if (
        (link.link_type === 'scattered' && link.quality === 'weak') ||
        link.link_type === 'direct'
    ) {
        dashed = true;
    }

    // Ghost Line Logic:
    // If the link is geometrically valid but radiometrically weak (Sub-Noise),
    // render it transparently to indicate "possibility" without "detection".
    const isGhost =
        (link.link_type === 'monostatic' || link.link_type === 'scattered') &&
        link.quality === 'weak';
    const opacity = isGhost ? 0.25 : 1.0;

    return (
        <Line
            points={points}
            color={color}
            lineWidth={1.5}
            dashed={dashed}
            dashScale={20}
            gapSize={10}
            transparent
            opacity={opacity}
        />
    );
}

// Component responsible for rendering a stack of labels at a specific 3D position
function LabelCluster({
    position,
    links,
}: {
    position: THREE.Vector3;
    links: VisualLink[];
}) {
    return (
        <Html position={position} center zIndexRange={[100, 0]}>
            <div
                style={{
                    display: 'flex',
                    flexDirection: 'column',
                    gap: '2px',
                    alignItems: 'center',
                    pointerEvents: 'none', // Ensure clicks pass through to camera controls
                    userSelect: 'none', // Ensure text cannot be dragged/selected
                }}
            >
                {links.map((link, i) => {
                    const color = getLinkColor(link);
                    return (
                        <div
                            key={i}
                            style={{
                                background: 'rgba(0,0,0,0.7)',
                                color: color,
                                padding: '2px 6px',
                                borderRadius: '4px',
                                fontSize: '10px',
                                whiteSpace: 'nowrap',
                                border: `1px solid ${color}`,
                                userSelect: 'none',
                            }}
                        >
                            {link.label}
                        </div>
                    );
                })}
            </div>
        </Html>
    );
}

/**
 * Renders radio frequency links between platforms based on physics calculations.
 * Handles grouping of overlapping labels to ensure readability.
 */
export default function LinkVisualizer() {
    const currentTime = useScenarioStore((state) => state.currentTime);
    const platforms = useScenarioStore((state) => state.platforms);
    const [links, setLinks] = useState<VisualLink[]>([]);

    useEffect(() => {
        let active = true;

        const fetchLinks = async () => {
            try {
                // Invoke the stateless physics calculation in the C++ core
                const data = await invoke<VisualLink[]>('get_preview_links', {
                    time: currentTime,
                });

                // Only update state if this effect is still active (prevents race conditions)
                if (active) {
                    setLinks(data);
                }
            } catch (e) {
                console.error('Link preview error:', e);
            }
        };

        void fetchLinks();

        return () => {
            active = false;
        };
    }, [currentTime, platforms]);

    // Group links based on their 3D midpoint to prevent label overlap
    const { clusters, flatLinks } = useMemo(() => {
        const map = new Map<
            string,
            { position: THREE.Vector3; links: VisualLink[] }
        >();

        links.forEach((link) => {
            const start = new THREE.Vector3(
                link.start[0],
                link.start[2],
                -link.start[1]
            );
            const end = new THREE.Vector3(
                link.end[0],
                link.end[2],
                -link.end[1]
            );
            const mid = start.clone().lerp(end, 0.5);

            // Quantize coordinates to group spatially close labels (0.1 unit tolerance)
            const key = `${mid.x.toFixed(1)}_${mid.y.toFixed(1)}_${mid.z.toFixed(
                1
            )}`;

            if (!map.has(key)) {
                map.set(key, { position: mid, links: [] });
            }
            map.get(key)!.links.push(link);
        });

        return { clusters: Array.from(map.values()), flatLinks: links };
    }, [links]);

    return (
        <group>
            {/* Render all lines individually for geometry */}
            {flatLinks.map((link, i) => (
                <LinkLine key={`line-${i}`} link={link} />
            ))}

            {/* Render aggregated label clusters */}
            {clusters.map((cluster, i) => (
                <LabelCluster
                    key={`cluster-${i}`}
                    position={cluster.position}
                    links={cluster.links}
                />
            ))}
        </group>
    );
}
