// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { Box, Typography, Slider, IconButton } from '@mui/material';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import SkipNextIcon from '@mui/icons-material/SkipNext';
import SkipPreviousIcon from '@mui/icons-material/SkipPrevious';

export default function Timeline() {
    return (
        <Box
            sx={{
                display: 'flex',
                alignItems: 'center',
                gap: 2,
                height: '100%',
            }}
        >
            <Box>
                <IconButton size="small">
                    <SkipPreviousIcon />
                </IconButton>
                <IconButton>
                    <PlayArrowIcon />
                </IconButton>
                <IconButton size="small">
                    <SkipNextIcon />
                </IconButton>
            </Box>
            <Typography variant="caption">0.00s</Typography>
            <Slider
                defaultValue={0}
                step={0.1}
                min={0}
                max={30}
                sx={{ mx: 1 }}
                valueLabelDisplay="auto"
                valueLabelFormat={(value) => `${value.toFixed(2)}s`}
            />
            <Typography variant="caption">30.00s</Typography>
        </Box>
    );
}
