// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { Box, Typography, Divider } from '@mui/material';
import { useScenarioStore, findItemInStore } from '@/stores/scenarioStore';
import { assertNever } from '@/utils/typeUtils';
import { GlobalParametersInspector } from './inspectors/GlobalParametersInspector';
import { PulseInspector } from './inspectors/PulseInspector';
import { TimingInspector } from './inspectors/TimingInspector';
import { AntennaInspector } from './inspectors/AntennaInspector';
import { PlatformInspector } from './inspectors/PlatformInspector';

function InspectorContent() {
    const selectedItem = useScenarioStore((state) =>
        findItemInStore(state, state.selectedItemId)
    );

    if (!selectedItem) {
        return (
            <Typography color="text.secondary">
                Select an item to see its properties.
            </Typography>
        );
    }

    const renderInspector = () => {
        switch (selectedItem.type) {
            case 'GlobalParameters':
                return <GlobalParametersInspector item={selectedItem} />;
            case 'Pulse':
                return <PulseInspector item={selectedItem} />;
            case 'Timing':
                return <TimingInspector item={selectedItem} />;
            case 'Antenna':
                return <AntennaInspector item={selectedItem} />;
            case 'Platform':
                return <PlatformInspector item={selectedItem} />;
            default:
                return assertNever(selectedItem);
        }
    };

    return (
        <Box>
            <Typography variant="overline" color="text.secondary">
                {selectedItem.type}
            </Typography>
            <Divider sx={{ my: 1 }} />
            {renderInspector()}
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
            }}
        >
            <Box sx={{ flexShrink: 0, px: 2, pt: 2, pb: 1 }}>
                <Typography variant="h6">Properties</Typography>
            </Box>
            <Divider sx={{ mx: 2 }} />

            <Box
                sx={{
                    flexGrow: 1,
                    overflowY: 'auto',
                    minHeight: 0,
                    p: 2,
                }}
            >
                <InspectorContent />
            </Box>
        </Box>
    );
}
