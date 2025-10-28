// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { Box, IconButton, Tooltip } from '@mui/material';
import CenterFocusStrongIcon from '@mui/icons-material/CenterFocusStrong';
import TravelExploreIcon from '@mui/icons-material/TravelExplore';
import VideocamIcon from '@mui/icons-material/Videocam';
import VideocamOffIcon from '@mui/icons-material/VideocamOff';
import { useScenarioStore } from '@/stores/scenarioStore';

export default function ViewControls() {
    const {
        frameScene,
        focusOnItem,
        toggleFollowItem,
        selectedItemId,
        viewControlAction,
    } = useScenarioStore();
    const isFollowing =
        viewControlAction.type === 'follow' &&
        viewControlAction.targetId === selectedItemId;

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
        </Box>
    );
}
