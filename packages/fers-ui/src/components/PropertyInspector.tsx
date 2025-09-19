// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { Box, Typography, Divider } from '@mui/material';
import { useScenarioStore, findItemInStore } from '@/stores/scenarioStore';
import { GlobalParametersInspector } from './inspectors/GlobalParametersInspector';
import { PulseInspector } from './inspectors/PulseInspector';
import { TimingInspector } from './inspectors/TimingInspector';
import { AntennaInspector } from './inspectors/AntennaInspector';
import { PlatformInspector } from './inspectors/PlatformInspector';

function InspectorContent() {
    const { selectedItemId } = useScenarioStore();
    const selectedItem = findItemInStore(selectedItemId);

    if (!selectedItem) {
        return (
            <Typography color="text.secondary">
                Select an item to see its properties.
            </Typography>
        );
    }

    let InspectorComponent;
    switch (selectedItem.type) {
        case 'GlobalParameters':
            InspectorComponent = (
                <GlobalParametersInspector item={selectedItem} />
            );
            break;
        case 'Pulse':
            InspectorComponent = <PulseInspector item={selectedItem} />;
            break;
        case 'Timing':
            InspectorComponent = <TimingInspector item={selectedItem} />;
            break;
        case 'Antenna':
            InspectorComponent = <AntennaInspector item={selectedItem} />;
            break;
        case 'Platform':
            InspectorComponent = <PlatformInspector item={selectedItem} />;
            break;
        default:
            return null;
    }

    return (
        <Box>
            <Typography variant="overline" color="text.secondary">
                {selectedItem.type}
            </Typography>
            <Divider sx={{ my: 1 }} />
            {InspectorComponent}
        </Box>
    );
}

export default function PropertyInspector() {
    return (
        <Box
            sx={{
                height: '100%',
                display: 'flex',
                flexDirection: 'column',
                // This component fills its parent (ResizablePanel)
            }}
        >
            {/* 1. Fixed Header */}
            <Box sx={{ flexShrink: 0, px: 2, pt: 2, pb: 1 }}>
                <Typography variant="h6">Properties</Typography>
            </Box>
            <Divider sx={{ mx: 2 }} />

            {/* 2. Scrollable Content Area */}
            <Box
                sx={{
                    flexGrow: 1, // Takes up all remaining space
                    overflowY: 'auto', // Enables vertical scrolling
                    minHeight: 0, // Crucial for flexbox scrolling
                    p: 2, // Apply padding inside the scrollable area
                }}
            >
                <InspectorContent />
            </Box>
        </Box>
    );
}
