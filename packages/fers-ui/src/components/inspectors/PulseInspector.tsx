// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import {
    Box,
    FormControl,
    InputLabel,
    MenuItem,
    Select,
    TextField,
} from '@mui/material';
import { useScenarioStore, Pulse } from '@/stores/scenarioStore';
import { NumberField, FileInput } from './InspectorControls';

interface PulseInspectorProps {
    item: Pulse;
}

export function PulseInspector({ item }: PulseInspectorProps) {
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
            <FormControl fullWidth size="small">
                <InputLabel>Type</InputLabel>
                <Select
                    label="Type"
                    value={item.pulseType}
                    onChange={(e) => handleChange('pulseType', e.target.value)}
                >
                    <MenuItem value="file">Pulse File</MenuItem>
                    <MenuItem value="cw">CW</MenuItem>
                </Select>
            </FormControl>
            <NumberField
                label="Power (W)"
                value={item.power}
                onChange={(v) => handleChange('power', v)}
            />
            <NumberField
                label="Carrier Frequency (Hz)"
                value={item.carrier}
                onChange={(v) => handleChange('carrier', v)}
            />
            {item.pulseType === 'file' && (
                <FileInput
                    label="Waveform File (.csv, .h5)"
                    value={item.filename}
                    onChange={(v) => handleChange('filename', v)}
                    filters={[
                        { name: 'Waveform', extensions: ['csv', 'h5'] },
                        { name: 'All Files', extensions: ['*'] },
                    ]}
                />
            )}
        </Box>
    );
}
