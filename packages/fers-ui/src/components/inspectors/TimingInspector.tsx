// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { Box, TextField } from '@mui/material';
import { useScenarioStore, Timing } from '@/stores/scenarioStore';
import { NumberField } from './InspectorControls';

export function TimingInspector({ item }: { item: Timing }) {
    const { updateItem } = useScenarioStore.getState();
    const handleChange = (path: string, value: unknown) =>
        updateItem(item.id, path, value);

    return (
        <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
            <TextField
                label="Name"
                variant="outlined"
                size="small"
                fullWidth
                value={item.name}
                onChange={(e) => handleChange('name', e.target.value)}
            />
            <NumberField
                label="Frequency (Hz)"
                value={item.frequency}
                onChange={(v) => handleChange('frequency', v)}
            />
        </Box>
    );
}
