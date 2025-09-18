// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import {
    Box,
    Typography,
    Divider,
    TextField,
    FormControl,
    InputLabel,
    Select,
    MenuItem,
} from '@mui/material';

export default function PropertyInspector() {
    return (
        <Box>
            <Typography variant="h6" gutterBottom sx={{ px: 1 }}>
                Properties
            </Typography>
            <Typography
                variant="body2"
                color="text.secondary"
                gutterBottom
                sx={{ px: 1 }}
            >
                AWACS
            </Typography>
            <Divider sx={{ my: 2 }} />
            <Box sx={{ px: 1 }}>
                <TextField
                    label="Name"
                    variant="outlined"
                    size="small"
                    fullWidth
                    defaultValue="AWACS"
                    sx={{ mb: 2 }}
                />
                <FormControl fullWidth size="small">
                    <InputLabel>Motion Path</InputLabel>
                    <Select label="Motion Path" value="linear">
                        <MenuItem value="static">Static</MenuItem>
                        <MenuItem value="linear">Linear Interpolation</MenuItem>
                        <MenuItem value="cubic">Cubic Interpolation</MenuItem>
                    </Select>
                </FormControl>
            </Box>
        </Box>
    );
}
