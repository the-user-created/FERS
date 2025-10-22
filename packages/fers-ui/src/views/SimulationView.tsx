// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import React, { useState, useEffect } from 'react';
import {
    Box,
    Typography,
    Grid,
    Card,
    CardContent,
    CardActions,
    Button,
    CircularProgress,
    Fade,
    LinearProgress,
} from '@mui/material';
import PlayCircleOutlineIcon from '@mui/icons-material/PlayCircleOutline';
import MapIcon from '@mui/icons-material/Map';
import { useScenarioStore } from '@/stores/scenarioStore';
import { invoke } from '@tauri-apps/api/core';
import { save } from '@tauri-apps/plugin-dialog';
import { listen } from '@tauri-apps/api/event';

interface ProgressState {
    message: string;
    current: number;
    total: number;
}

export const SimulationView = React.memo(function SimulationView() {
    const isSimulating = useScenarioStore((state) => state.isSimulating);
    const setIsSimulating = useScenarioStore((state) => state.setIsSimulating);
    const showError = useScenarioStore((state) => state.showError);
    const [isGeneratingKml, setIsGeneratingKml] = useState(false);
    const [progress, setProgress] = useState<ProgressState | null>(null);

    useEffect(() => {
        const unlistenSimComplete = listen<void>('simulation-complete', () => {
            console.log('Simulation completed successfully.');
            setIsSimulating(false);
            setProgress(null);
        });

        const unlistenSimError = listen<string>('simulation-error', (event) => {
            const errorMessage = `Simulation failed: ${event.payload}`;
            console.error(errorMessage);
            showError(errorMessage);
            setIsSimulating(false);
            setProgress(null);
        });

        const unlistenSimProgress = listen<ProgressState>(
            'simulation-progress',
            (event) => {
                setProgress(event.payload);
            }
        );

        const unlistenKmlComplete = listen<string>(
            'kml-generation-complete',
            (event) => {
                console.log('KML generated successfully at:', event.payload);
                setIsGeneratingKml(false);
            }
        );

        const unlistenKmlError = listen<string>(
            'kml-generation-error',
            (event) => {
                const errorMessage = `KML generation failed: ${event.payload}`;
                console.error(errorMessage);
                showError(errorMessage);
                setIsGeneratingKml(false);
            }
        );

        // Cleanup function to remove listeners when the component unmounts
        return () => {
            Promise.all([
                unlistenSimComplete,
                unlistenSimError,
                unlistenSimProgress,
                unlistenKmlComplete,
                unlistenKmlError,
            ]).then((unlisteners) => {
                unlisteners.forEach((unlisten) => unlisten());
            });
        };
    }, [setIsSimulating, showError]);

    const handleRunSimulation = async () => {
        setProgress(null);
        setIsSimulating(true);
        try {
            // Ensure the C++ backend has the latest scenario from the UI
            await useScenarioStore.getState().syncBackend();
            await invoke('run_simulation');
        } catch (err) {
            const errorMessage =
                err instanceof Error ? err.message : String(err);
            console.error('Failed to invoke simulation:', errorMessage);
            showError(`Failed to start simulation: ${errorMessage}`);
            setIsSimulating(false); // Stop on invocation failure
        }
    };

    const handleGenerateKml = async () => {
        try {
            const outputPath = await save({
                title: 'Save KML File',
                filters: [{ name: 'KML File', extensions: ['kml'] }],
            });

            if (outputPath) {
                setIsGeneratingKml(true);
                // Ensure the C++ backend has the latest scenario from the UI
                await useScenarioStore.getState().syncBackend();
                await invoke('generate_kml', { outputPath });
            }
        } catch (err) {
            const errorMessage =
                err instanceof Error ? err.message : String(err);
            console.error('Failed to invoke KML generation:', errorMessage);
            showError(`Failed to start KML generation: ${errorMessage}`);
            setIsGeneratingKml(false); // Stop on invocation failure
        }
    };

    return (
        <Box sx={{ p: 4, height: '100%', overflowY: 'auto' }}>
            <Typography variant="h4" gutterBottom>
                Simulation Runner
            </Typography>
            <Typography variant="body1" color="text.secondary" sx={{ mb: 4 }}>
                Execute the configured scenario or generate a geographical
                visualization. Ensure your scenario is fully configured before
                proceeding.
            </Typography>

            <Grid container spacing={4} sx={{ width: '100%' }}>
                <Grid size={{ xs: 12, md: 6 }}>
                    <Card sx={{ height: '100%' }}>
                        <CardContent>
                            <Typography variant="h5" component="div">
                                Run Full Simulation
                            </Typography>
                            <Typography sx={{ mt: 1.5 }} color="text.secondary">
                                Executes the entire simulation based on the
                                current scenario settings. This is a
                                computationally intensive process that will
                                generate output files (HDF5, CSV, XML) as
                                configured in the Global Parameters.
                            </Typography>
                        </CardContent>
                        <CardActions sx={{ p: 2 }}>
                            <Button
                                variant="contained"
                                size="large"
                                startIcon={
                                    isSimulating ? (
                                        <CircularProgress
                                            size={24}
                                            color="inherit"
                                        />
                                    ) : (
                                        <PlayCircleOutlineIcon />
                                    )
                                }
                                disabled={isSimulating || isGeneratingKml}
                                onClick={handleRunSimulation}
                            >
                                {isSimulating ? 'Running...' : 'Run Simulation'}
                            </Button>
                        </CardActions>
                    </Card>
                </Grid>
                <Grid size={{ xs: 12, md: 6 }}>
                    <Card sx={{ height: '100%' }}>
                        <CardContent>
                            <Typography variant="h5" component="div">
                                Generate KML
                            </Typography>
                            <Typography sx={{ mt: 1.5 }} color="text.secondary">
                                Creates a KML file from the scenario&apos;s
                                platform motion paths and antenna pointings.
                                This allows for quick visualization in
                                applications like Google Earth without running
                                the full signal-level simulation.
                            </Typography>
                        </CardContent>
                        <CardActions sx={{ p: 2 }}>
                            <Button
                                variant="outlined"
                                size="large"
                                startIcon={
                                    isGeneratingKml ? (
                                        <CircularProgress
                                            size={24}
                                            color="inherit"
                                        />
                                    ) : (
                                        <MapIcon />
                                    )
                                }
                                disabled={isSimulating || isGeneratingKml}
                                onClick={handleGenerateKml}
                            >
                                {isGeneratingKml
                                    ? 'Generating...'
                                    : 'Generate KML'}
                            </Button>
                        </CardActions>
                    </Card>
                </Grid>
            </Grid>

            <Fade in={isSimulating}>
                <Box
                    sx={{
                        mt: 4,
                        p: 2,
                        backgroundColor: 'action.hover',
                        borderRadius: 1,
                    }}
                >
                    <Typography
                        variant="h6"
                        sx={{ mb: 1, textAlign: 'center' }}
                    >
                        {progress
                            ? progress.message
                            : 'Preparing simulation...'}
                    </Typography>
                    {progress && progress.total > 0 && (
                        <Box
                            sx={{
                                display: 'flex',
                                alignItems: 'center',
                                mt: 2,
                            }}
                        >
                            <Box sx={{ width: '100%', mr: 1 }}>
                                <LinearProgress
                                    variant="determinate"
                                    value={
                                        (progress.current / progress.total) *
                                        100
                                    }
                                />
                            </Box>
                            <Box sx={{ minWidth: 40 }}>
                                <Typography
                                    variant="body2"
                                    color="text.secondary"
                                >{`${Math.round(
                                    (progress.current / progress.total) * 100
                                )}%`}</Typography>
                            </Box>
                        </Box>
                    )}
                    <Typography
                        variant="body2"
                        color="text.secondary"
                        sx={{ mt: 1, textAlign: 'center' }}
                    >
                        This may take several minutes depending on scenario
                        complexity.
                    </Typography>
                </Box>
            </Fade>
        </Box>
    );
});
