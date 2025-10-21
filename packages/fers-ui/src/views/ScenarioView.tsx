// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import React from 'react';
import { Box } from '@mui/material';
import { Canvas } from '@react-three/fiber';
import WorldView from '@/components/WorldView';
import SceneTree from '@/components/SceneTree';
import PropertyInspector from '@/components/PropertyInspector';
import Timeline from '@/components/Timeline';
import { useScenarioStore } from '@/stores/scenarioStore';

/**
 * ScenarioView is the primary workbench for building and visualizing 3D scenes.
 */
export const ScenarioView = React.memo(function ScenarioView() {
    const isSimulating = useScenarioStore((state) => state.isSimulating);

    return (
        <Box
            sx={{
                display: 'flex',
                height: '100%',
                width: '100%',
                overflow: 'hidden',
                position: 'relative', // Establish positioning context
                pointerEvents: isSimulating ? 'none' : 'auto',
                opacity: isSimulating ? 0.5 : 1,
                transition: 'opacity 0.3s ease-in-out',
            }}
        >
            <Box
                sx={{
                    width: 280,
                    flexShrink: 0,
                    borderRight: 1,
                    borderColor: 'divider',
                }}
            >
                <SceneTree />
            </Box>

            <Box
                sx={{
                    flex: 1,
                    display: 'flex',
                    flexDirection: 'column',
                    minWidth: 0, // Allow flex item to shrink below content size
                    overflow: 'hidden', // Prevent overflow
                }}
            >
                <Box
                    sx={{
                        flex: 1,
                        position: 'relative',
                        minHeight: 0, // Allow flex item to shrink
                        overflow: 'hidden',
                    }}
                >
                    <Canvas
                        shadows
                        camera={{
                            position: [100, 100, 100],
                            fov: 25,
                            near: 1,
                            far: 100000,
                        }}
                        gl={{
                            logarithmicDepthBuffer: true,
                        }}
                    >
                        <WorldView />
                    </Canvas>
                </Box>
                <Box
                    sx={{
                        height: 100,
                        flexShrink: 0,
                        borderTop: 1,
                        borderColor: 'divider',
                    }}
                >
                    <Timeline />
                </Box>
            </Box>

            <Box
                sx={{
                    width: 320,
                    flexShrink: 0,
                    borderLeft: 1,
                    borderColor: 'divider',
                }}
            >
                <PropertyInspector />
            </Box>
        </Box>
    );
});
