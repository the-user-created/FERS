// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useEffect, useMemo, useRef } from 'react';
import { Box, Typography, Slider, IconButton } from '@mui/material';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import PauseIcon from '@mui/icons-material/Pause';
import { useScenarioStore } from '@/stores/scenarioStore';

export default function Timeline() {
    const {
        globalParameters,
        isPlaying,
        currentTime,
        togglePlayPause,
        setCurrentTime,
        targetPlaybackDuration,
    } = useScenarioStore();

    const animationFrameRef = useRef(0);
    const lastTimeRef = useRef(performance.now());

    // Calculate dynamic values based on simulation duration and user settings.
    const { simulationDuration, speedFactor, sliderStep } = useMemo(() => {
        const duration = Math.max(
            0,
            globalParameters.end - globalParameters.start
        );

        let realPlaybackDuration: number;

        if (targetPlaybackDuration !== null && targetPlaybackDuration > 0) {
            // User override is active
            realPlaybackDuration = targetPlaybackDuration;
        } else {
            // Default behavior
            if (duration > 0 && duration < 5) {
                // Slow down short simulations to last at least 5 seconds.
                realPlaybackDuration = 5;
            } else {
                // Otherwise, play in real-time (or not at all if duration is 0).
                realPlaybackDuration = duration;
            }
        }

        const factor =
            realPlaybackDuration > 0 ? duration / realPlaybackDuration : 0;
        const step = duration > 0 ? duration / 1000 : 0.01;

        return {
            simulationDuration: duration,
            speedFactor: factor,
            sliderStep: step,
        };
    }, [globalParameters.start, globalParameters.end, targetPlaybackDuration]);

    useEffect(() => {
        const animate = (now: number) => {
            const deltaTime = (now - lastTimeRef.current) / 1000; // in seconds
            lastTimeRef.current = now;

            setCurrentTime((prevTime) => {
                const nextTime = prevTime + deltaTime * speedFactor;
                // Loop simulation when it reaches the end
                return nextTime >= globalParameters.end
                    ? globalParameters.start
                    : nextTime;
            });

            animationFrameRef.current = requestAnimationFrame(animate);
        };

        // Only run the animation if playing AND there is a duration to play through.
        if (isPlaying && simulationDuration > 0) {
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
        simulationDuration,
        speedFactor,
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
                <IconButton onClick={togglePlayPause}>
                    {isPlaying ? <PauseIcon /> : <PlayArrowIcon />}
                </IconButton>
            </Box>
            <Typography
                variant="caption"
                sx={{ flexShrink: 0, width: '4em', textAlign: 'center' }}
            >
                {currentTime.toFixed(2)}s
            </Typography>
            <Slider
                value={currentTime}
                onChange={handleSliderChange}
                step={sliderStep}
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
            <Typography
                variant="caption"
                sx={{ flexShrink: 0, width: '4em', textAlign: 'right' }}
            >
                {globalParameters.end.toFixed(2)}s
            </Typography>
        </Box>
    );
}
