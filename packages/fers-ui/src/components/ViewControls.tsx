// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useState } from 'react';
import {
    Box,
    IconButton,
    Tooltip,
    Dialog,
    DialogTitle,
    DialogContent,
    DialogActions,
    Button,
    FormGroup,
    FormControlLabel,
    Checkbox,
} from '@mui/material';
import CenterFocusStrongIcon from '@mui/icons-material/CenterFocusStrong';
import TravelExploreIcon from '@mui/icons-material/TravelExplore';
import VideocamIcon from '@mui/icons-material/Videocam';
import VideocamOffIcon from '@mui/icons-material/VideocamOff';
import LayersIcon from '@mui/icons-material/Layers';
import { useScenarioStore, VisualizationLayers } from '@/stores/scenarioStore';

export default function ViewControls() {
    const {
        frameScene,
        focusOnItem,
        toggleFollowItem,
        selectedItemId,
        viewControlAction,
        visibility,
        toggleLayer,
    } = useScenarioStore();

    const [layersOpen, setLayersOpen] = useState(false);

    const isFollowing =
        viewControlAction.type === 'follow' &&
        viewControlAction.targetId === selectedItemId;

    const layerLabels: Record<keyof VisualizationLayers, string> = {
        showPlatforms: 'Platforms (Spheres)',
        showAxes: 'Platform Axes',
        showVelocities: 'Velocity Arrows',
        showBoresights: 'Boresight Arrows',
        showPatterns: 'Antenna Patterns',
        showMotionPaths: 'Motion Paths',
        showLinks: 'RF Links',
    };

    return (
        <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1 }}>
            <Tooltip title="Frame Scene" placement="right">
                <IconButton
                    onClick={frameScene}
                    color="primary"
                    sx={{ bgcolor: 'background.paper' }}
                >
                    <TravelExploreIcon />
                </IconButton>
            </Tooltip>
            <Tooltip title="Focus on Selected" placement="right">
                <span>
                    <IconButton
                        onClick={() =>
                            selectedItemId && focusOnItem(selectedItemId)
                        }
                        disabled={!selectedItemId}
                        color="primary"
                        sx={{ bgcolor: 'background.paper' }}
                    >
                        <CenterFocusStrongIcon />
                    </IconButton>
                </span>
            </Tooltip>
            <Tooltip
                title={isFollowing ? 'Unfollow Selected' : 'Follow Selected'}
                placement="right"
            >
                <span>
                    <IconButton
                        onClick={() =>
                            selectedItemId && toggleFollowItem(selectedItemId)
                        }
                        disabled={!selectedItemId}
                        color={isFollowing ? 'secondary' : 'primary'}
                        sx={{ bgcolor: 'background.paper' }}
                    >
                        {isFollowing ? <VideocamOffIcon /> : <VideocamIcon />}
                    </IconButton>
                </span>
            </Tooltip>

            {/* Layers Dialog Toggle */}
            <Tooltip title="Visualization Layers" placement="right">
                <IconButton
                    onClick={() => setLayersOpen(true)}
                    color="primary"
                    sx={{ bgcolor: 'background.paper' }}
                >
                    <LayersIcon />
                </IconButton>
            </Tooltip>

            <Dialog open={layersOpen} onClose={() => setLayersOpen(false)}>
                <DialogTitle>View Layers</DialogTitle>
                <DialogContent>
                    <FormGroup>
                        {(
                            Object.keys(visibility) as Array<
                                keyof VisualizationLayers
                            >
                        ).map((key) => (
                            <FormControlLabel
                                key={key}
                                control={
                                    <Checkbox
                                        checked={visibility[key]}
                                        onChange={() => toggleLayer(key)}
                                    />
                                }
                                label={layerLabels[key]}
                            />
                        ))}
                    </FormGroup>
                </DialogContent>
                <DialogActions>
                    <Button onClick={() => setLayersOpen(false)}>Close</Button>
                </DialogActions>
            </Dialog>
        </Box>
    );
}
