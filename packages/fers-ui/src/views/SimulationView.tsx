// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { Box, Typography } from '@mui/material';

export function SimulationView() {
    return (
        <Box sx={{ p: 3 }}>
            <Typography variant="h4">Simulation Runner</Typography>
            <Typography>
                Configure simulation parameters and export to FERS XML.
            </Typography>
        </Box>
    );
}
