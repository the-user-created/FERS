// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useState, useEffect, useMemo } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { Line, Html } from '@react-three/drei';
import * as THREE from 'three';
import {
    useScenarioStore,
    Platform,
    calculateInterpolatedPosition,
} from '@/stores/scenarioStore';
import { Tooltip, Box, Typography } from '@mui/material';

const TYPE_MAP = ['monostatic', 'illuminator', 'scattered', 'direct'] as const;
const QUALITY_MAP = ['strong', 'weak'] as const;
const COLORS = {
    monostatic: { strong: '#00e676', weak: '#f44336' },
    illuminator: '#ffeb3b',
    scattered: '#00bcd4',
    direct: '#9c27b0',
};

interface VisualLink {
    link_type: 'monostatic' | 'illuminator' | 'scattered' | 'direct';
    quality: 'strong' | 'weak';
    start: THREE.Vector3;
    end: THREE.Vector3;
    label: string;
    source_name: string;
    dest_name: string;
    origin_name: string;
    distance: number;
}

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

function LinkLine({ link }: { link: VisualLink }) {
    const points = useMemo(
        () => [link.start, link.end] as [THREE.Vector3, THREE.Vector3],
        [link.start, link.end]
    );
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

function LabelItem({ link, color }: { link: VisualLink; color: string }) {
    return (
        <Tooltip
            arrow
            placement="top"
            title={
                <Box sx={{ p: 0.5 }}>
                    <Typography variant="subtitle2" sx={{ fontWeight: 'bold' }}>
                        Link Details
                    </Typography>
                    <Typography variant="caption" display="block">
                        <b>Path Segment:</b> {link.source_name} →{' '}
                        {link.dest_name}
                    </Typography>
                    {/* Only show Origin if it's a scattered link, otherwise it's redundant */}
                    {link.link_type === 'scattered' && (
                        <Typography variant="caption" display="block">
                            <b>Illuminator:</b> {link.origin_name}
                        </Typography>
                    )}
                    <Typography variant="caption" display="block">
                        <b>Type:</b> {link.link_type}
                    </Typography>
                    <Typography variant="caption" display="block">
                        <b>Distance:</b> {(link.distance / 1000).toFixed(2)} km
                    </Typography>
                    <Typography variant="caption" display="block">
                        <b>Value:</b> {link.label}
                    </Typography>
                </Box>
            }
        >
            <div
                style={{
                    background: 'rgba(0,0,0,0.7)',
                    color: color,
                    padding: '2px 6px',
                    borderRadius: '4px',
                    fontSize: '10px',
                    whiteSpace: 'nowrap',
                    border: `1px solid ${color}`,
                    userSelect: 'none',
                    cursor: 'help',
                    pointerEvents: 'auto',
                }}
            >
                {link.label}
            </div>
        </Tooltip>
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
                    pointerEvents: 'none',
                }}
            >
                {links.map((link, i) => {
                    const color = getLinkColor(link);
                    return <LabelItem key={i} link={link} color={color} />;
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

    // Build a lookup map: Component Name -> Parent Platform
    const componentToPlatform = useMemo(() => {
        const map = new Map<string, Platform>();
        platforms.forEach((p) => {
            // Map platform components (Tx, Rx, Tgt) to the platform
            p.components.forEach((c) => map.set(c.name, p));
        });
        return map;
    }, [platforms]);

    useEffect(() => {
        let active = true;

        const fetchLinks = async () => {
            try {
                // Fetch SoA (Structure of Arrays)
                const [numericData, stringData] = await invoke<
                    [number[], string[]]
                >('get_preview_links', {
                    time: currentTime,
                });

                if (!active) return;

                const reconstructedLinks: VisualLink[] = [];
                const count = numericData.length / 2;

                for (let i = 0; i < count; i++) {
                    const typeIdx = numericData[i * 2];
                    const qualIdx = numericData[i * 2 + 1];
                    const sourceName = stringData[i * 4];
                    const destName = stringData[i * 4 + 1];
                    const originName = stringData[i * 4 + 2];
                    const label = stringData[i * 4 + 3];

                    const sourcePlat = componentToPlatform.get(sourceName);
                    const destPlat = componentToPlatform.get(destName);

                    if (sourcePlat && destPlat) {
                        // Client-side interpolation reuse
                        const startPos = calculateInterpolatedPosition(
                            sourcePlat,
                            currentTime
                        );
                        const endPos = calculateInterpolatedPosition(
                            destPlat,
                            currentTime
                        );
                        const dist = startPos.distanceTo(endPos); // Distance in 3D units (meters in FERS/ThreeJS mapping)

                        reconstructedLinks.push({
                            link_type: TYPE_MAP[typeIdx],
                            quality: QUALITY_MAP[qualIdx],
                            start: startPos,
                            end: endPos,
                            distance: dist,
                            label,
                            source_name: sourceName,
                            dest_name: destName,
                            origin_name: originName,
                        });
                    }
                }
                setLinks(reconstructedLinks);
            } catch (e) {
                console.error('Link preview error:', e);
            }
        };

        void fetchLinks();

        return () => {
            active = false;
        };
    }, [currentTime, componentToPlatform]);

    // Group links based on their 3D midpoint to prevent label overlap
    const { clusters, flatLinks } = useMemo(() => {
        const map = new Map<
            string,
            { position: THREE.Vector3; links: VisualLink[] }
        >();

        links.forEach((link) => {
            const mid = link.start.clone().lerp(link.end, 0.5);
            const key = `${mid.x.toFixed(1)}_${mid.y.toFixed(1)}_${mid.z.toFixed(1)}`;
            if (!map.has(key)) map.set(key, { position: mid, links: [] });
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
