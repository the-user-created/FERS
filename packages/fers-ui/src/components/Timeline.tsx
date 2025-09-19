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
                px: 2, // Add horizontal padding
                overflow: 'hidden', // Prevent overflow
            }}
        >
            <Box sx={{ flexShrink: 0 }}>
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
            <Typography variant="caption" sx={{ flexShrink: 0 }}>
                0.00s
            </Typography>
            <Slider
                defaultValue={0}
                step={0.1}
                min={0}
                max={30}
                sx={{
                    mx: 1,
                    flexGrow: 1,
                    minWidth: 50, // Ensure minimum usable width
                }}
                valueLabelDisplay="auto"
                valueLabelFormat={(value) => `${value.toFixed(2)}s`}
            />
            <Typography variant="caption" sx={{ flexShrink: 0 }}>
                30.00s
            </Typography>
        </Box>
    );
}
