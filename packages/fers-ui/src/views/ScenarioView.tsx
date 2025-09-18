// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { Box } from '@mui/material';
import { Canvas } from '@react-three/fiber';
import WorldView from '@/components/WorldView';
import ResizablePanel from '@/components/ResizablePanel';
import SceneTree from '@/components/SceneTree';
import PropertyInspector from '@/components/PropertyInspector';
import Timeline from '@/components/Timeline';

/**
 * ScenarioView is the primary workbench for building and visualizing 3D scenes.
 */
export function ScenarioView() {
    return (
        <Box
            sx={{
                display: 'flex',
                height: '100%',
                width: '100%',
                overflow: 'hidden',
            }}
        >
            <ResizablePanel
                direction="horizontal"
                side="left"
                initialSize={280}
                minSize={200}
                maxSize={500}
                sx={{ borderRight: 1, borderColor: 'divider' }}
            >
                <SceneTree />
            </ResizablePanel>

            <Box
                sx={{
                    flex: 1,
                    display: 'flex',
                    flexDirection: 'column',
                    minWidth: 0,
                }}
            >
                <Box sx={{ flex: 1, position: 'relative' }}>
                    <Canvas camera={{ position: [10, 10, 10], fov: 25 }}>
                        <WorldView />
                    </Canvas>
                </Box>
                <ResizablePanel
                    direction="vertical"
                    side="bottom"
                    initialSize={100}
                    minSize={60}
                    maxSize={400}
                    sx={{ borderTop: 1, borderColor: 'divider' }}
                >
                    <Timeline />
                </ResizablePanel>
            </Box>

            <ResizablePanel
                direction="horizontal"
                side="right"
                initialSize={320}
                minSize={250}
                maxSize={600}
                sx={{ borderLeft: 1, borderColor: 'divider' }}
            >
                <PropertyInspector />
            </ResizablePanel>
        </Box>
    );
}
