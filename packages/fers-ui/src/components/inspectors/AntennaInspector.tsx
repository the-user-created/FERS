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
import { useScenarioStore, Antenna } from '@/stores/scenarioStore';
import { FileInput } from './InspectorControls';

export function AntennaInspector({ item }: { item: Antenna }) {
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
                <InputLabel>Pattern</InputLabel>
                <Select
                    label="Pattern"
                    value={item.pattern}
                    onChange={(e) => handleChange('pattern', e.target.value)}
                >
                    <MenuItem value="sinc">Sinc</MenuItem>
                    <MenuItem value="gaussian">Gaussian</MenuItem>
                    <MenuItem value="squarehorn">Square Horn</MenuItem>
                    <MenuItem value="parabolic">Parabolic</MenuItem>
                    <MenuItem value="xml">XML</MenuItem>
                    <MenuItem value="file">File (H5)</MenuItem>
                </Select>
            </FormControl>
            {(item.pattern === 'xml' || item.pattern === 'file') && (
                <FileInput
                    label="Pattern File"
                    value={item.filename}
                    onChange={(v) => handleChange('filename', v)}
                    filters={[
                        {
                            name: 'Antenna Pattern',
                            extensions:
                                item.pattern === 'xml' ? ['xml'] : ['h5'],
                        },
                        { name: 'All Files', extensions: ['*'] },
                    ]}
                />
            )}
        </Box>
    );
}
