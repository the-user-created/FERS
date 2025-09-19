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
            <Typography sx={{ p: 2 }} color="text.secondary">
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
        <Box sx={{ p: 1 }}>
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
                overflow: 'hidden',
            }}
        >
            <Typography
                variant="h6"
                gutterBottom
                sx={{ px: 1, pt: 0.5, mb: 1 }}
            >
                Properties
            </Typography>
            <Box sx={{ flex: 1, overflow: 'auto', minHeight: 0 }}>
                <InspectorContent />
            </Box>
        </Box>
    );
}
