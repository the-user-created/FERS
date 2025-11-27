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

function LinkSegment({ link }: { link: VisualLink }) {
    const [hovered, setHovered] = useState(false);

    // Coordinate Transform: Physics (ENU) -> Visual (Three.js)
    // ENU: X=East, Y=North, Z=Up
    // Three.js: X=Right, Y=Up, Z=Back (North is -Z)
    const start = useMemo(
        () => new THREE.Vector3(link.start[0], link.start[2], -link.start[1]),
        [link.start]
    );
    const end = useMemo(
        () => new THREE.Vector3(link.end[0], link.end[2], -link.end[1]),
        [link.end]
    );

    const points = useMemo(() => [start, end], [start, end]);
    const midPoint = useMemo(() => start.clone().lerp(end, 0.5), [start, end]);

    let color = '#ffffff';
    let dashed = false;

    if (link.link_type === 'monostatic') {
        color =
            link.quality === 'strong'
                ? COLORS.monostatic.strong
                : COLORS.monostatic.weak;
    } else if (link.link_type === 'illuminator') {
        color = COLORS.illuminator;
    } else if (link.link_type === 'scattered') {
        color = COLORS.scattered;
        // Scattered weak links are ghosts
        if (link.quality === 'weak') dashed = true;
    } else if (link.link_type === 'direct') {
        color = COLORS.direct;
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
        <group>
            <Line
                points={points}
                color={color}
                lineWidth={hovered ? 3 : 1.5}
                dashed={dashed}
                dashScale={20} // Adjust based on scene scale
                gapSize={10}
                transparent
                opacity={opacity}
                onPointerOver={() => setHovered(true)}
                onPointerOut={() => setHovered(false)}
            />

            {/* Label */}
            <Html position={midPoint} center zIndexRange={[100, 0]}>
                <div
                    style={{
                        background: 'rgba(0,0,0,0.7)',
                        color: color,
                        padding: '2px 6px',
                        borderRadius: '4px',
                        fontSize: '10px',
                        whiteSpace: 'nowrap',
                        pointerEvents: 'none',
                        border: `1px solid ${color}`,
                        userSelect: 'none',
                    }}
                >
                    {link.label}
                </div>
            </Html>
        </group>
    );
}

/**
 * Renders radio frequency links between platforms based on physics calculations.
 *
 * This component is reactive: it only communicates with the backend when the
 * simulation time changes (playback) or the scenario changes (editing).
 * When paused, it remains idle, preventing "RCS flicker".
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
    // Dependency Array Explanation:
    // 1. currentTime: Triggers updates during playback (e.g., 60fps). When paused, this doesn't change, stopping updates.
    // 2. platforms: Triggers updates when user modifies the scene (e.g., changes antenna, moves object).

    return (
        <group>
            {links.map((link, i) => (
                // Key by index is acceptable here as the list is fully regenerated every frame
                <LinkSegment key={i} link={link} />
            ))}
        </group>
    );
}
