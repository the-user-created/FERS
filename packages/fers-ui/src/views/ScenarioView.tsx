// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import React, { useRef, useState } from 'react';
import { Box, IconButton, Tooltip } from '@mui/material';
import ChevronLeftIcon from '@mui/icons-material/ChevronLeft';
import { Canvas } from '@react-three/fiber';
import {
    Panel,
    PanelGroup,
    PanelResizeHandle,
    type ImperativePanelGroupHandle,
} from 'react-resizable-panels';
import WorldView from '@/components/WorldView';
import SceneTree from '@/components/SceneTree';
import PropertyInspector from '@/components/PropertyInspector';
import Timeline from '@/components/Timeline';
import { useScenarioStore } from '@/stores/scenarioStore';

/**
 * A styled resize handle for the resizable panels.
 */
function ResizeHandle() {
    return (
        <PanelResizeHandle>
            <Box
                sx={{
                    width: '2px',
                    height: '100%',
                    backgroundColor: 'divider',
                    transition: 'background-color 0.2s ease-in-out',
                    '&[data-resize-handle-state="drag"]': {
                        backgroundColor: 'primary.main',
                    },
                    '&:hover': {
                        backgroundColor: 'primary.light',
                    },
                }}
            />
        </PanelResizeHandle>
    );
}

/**
 * ScenarioView is the primary workbench for building and visualizing 3D scenes.
 */
export const ScenarioView = React.memo(function ScenarioView() {
    const isSimulating = useScenarioStore((state) => state.isSimulating);
    const [isInspectorCollapsed, setIsInspectorCollapsed] = useState(false);
    const panelGroupRef = useRef<ImperativePanelGroupHandle>(null);

    const handleExpandInspector = () => {
        // Restore panels to their default sizes: [SceneTree, Main, Inspector]
        panelGroupRef.current?.setLayout([25, 50, 25]);
    };

    return (
        <Box
            sx={{
                height: '100%',
                width: '100%',
                overflow: 'hidden',
                position: 'relative', // Establish positioning context
                pointerEvents: isSimulating ? 'none' : 'auto',
                opacity: isSimulating ? 0.5 : 1,
                transition: 'opacity 0.3s ease-in-out',
            }}
        >
            <PanelGroup direction="horizontal" ref={panelGroupRef}>
                <Panel id="scene-tree" defaultSize={25} minSize={20} order={1}>
                    <SceneTree />
                </Panel>

                <ResizeHandle />

                <Panel id="main-content" minSize={30} order={2}>
                    <Box
                        sx={{
                            height: '100%',
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
                </Panel>

                <ResizeHandle />

                <Panel
                    id="property-inspector"
                    collapsible
                    collapsedSize={0}
                    onCollapse={() => setIsInspectorCollapsed(true)}
                    onExpand={() => setIsInspectorCollapsed(false)}
                    defaultSize={25}
                    minSize={5}
                    order={3}
                >
                    <PropertyInspector />
                </Panel>
            </PanelGroup>

            {isInspectorCollapsed && (
                <Tooltip title="Show Properties">
                    <IconButton
                        onClick={handleExpandInspector}
                        sx={{
                            position: 'absolute',
                            right: 0,
                            top: '50%',
                            transform: 'translateY(-50%)',
                            zIndex: 1,
                            bgcolor: 'background.paper',
                            border: 1,
                            borderColor: 'divider',
                            borderRight: 'none',
                            borderTopRightRadius: 0,
                            borderBottomRightRadius: 0,
                            '&:hover': {
                                bgcolor: 'action.hover',
                            },
                        }}
                    >
                        <ChevronLeftIcon />
                    </IconButton>
                </Tooltip>
            )}
        </Box>
    );
});
