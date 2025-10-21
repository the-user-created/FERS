// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useEffect, useRef } from 'react';
import { Box, Typography, Slider, IconButton } from '@mui/material';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import PauseIcon from '@mui/icons-material/Pause';
import SkipNextIcon from '@mui/icons-material/SkipNext';
import SkipPreviousIcon from '@mui/icons-material/SkipPrevious';
import { useScenarioStore } from '@/stores/scenarioStore';

export default function Timeline() {
    const {
        globalParameters,
        isPlaying,
        currentTime,
        togglePlayPause,
        setCurrentTime,
    } = useScenarioStore();

    const animationFrameRef = useRef(0);
    const lastTimeRef = useRef(performance.now());

    useEffect(() => {
        const animate = (now: number) => {
            const deltaTime = (now - lastTimeRef.current) / 1000; // in seconds
            lastTimeRef.current = now;

            setCurrentTime((prevTime) => {
                const nextTime = prevTime + deltaTime;
                // Loop simulation when it reaches the end
                return nextTime > globalParameters.end
                    ? globalParameters.start
                    : nextTime;
            });

            animationFrameRef.current = requestAnimationFrame(animate);
        };

        if (isPlaying) {
            lastTimeRef.current = performance.now();
            animationFrameRef.current = requestAnimationFrame(animate);
        } else {
            cancelAnimationFrame(animationFrameRef.current);
        }

        return () => cancelAnimationFrame(animationFrameRef.current);
    }, [
        isPlaying,
        setCurrentTime,
        globalParameters.start,
        globalParameters.end,
    ]);

    const handleSliderChange = (_event: Event, newValue: number | number[]) => {
        setCurrentTime(newValue as number);
    };

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
                <IconButton onClick={togglePlayPause}>
                    {isPlaying ? <PauseIcon /> : <PlayArrowIcon />}
                </IconButton>
                <IconButton size="small">
                    <SkipNextIcon />
                </IconButton>
            </Box>
            <Typography variant="caption" sx={{ flexShrink: 0 }}>
                {globalParameters.start.toFixed(2)}s
            </Typography>
            <Slider
                value={currentTime}
                onChange={handleSliderChange}
                step={0.01}
                min={globalParameters.start}
                max={globalParameters.end}
                sx={{
                    mx: 1,
                    flexGrow: 1,
                    minWidth: 50, // Ensure minimum usable width
                }}
                valueLabelDisplay="auto"
                valueLabelFormat={(value) => `${value.toFixed(2)}s`}
            />
            <Typography variant="caption" sx={{ flexShrink: 0 }}>
                {globalParameters.end.toFixed(2)}s
            </Typography>
        </Box>
    );
}
