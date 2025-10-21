// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { SimpleTreeView, TreeItem } from '@mui/x-tree-view';
import { useScenarioStore } from '@/stores/scenarioStore';
import { Box, Typography, IconButton, Tooltip, Divider } from '@mui/material';

import ExpandMoreIcon from '@mui/icons-material/ExpandMore';
import ChevronRightIcon from '@mui/icons-material/ChevronRight';
import PublicIcon from '@mui/icons-material/Public';
import WavesIcon from '@mui/icons-material/Waves';
import TimerIcon from '@mui/icons-material/Timer';
import SettingsInputAntennaIcon from '@mui/icons-material/SettingsInputAntenna';
import FlightIcon from '@mui/icons-material/Flight';
import AddIcon from '@mui/icons-material/Add';
import RemoveIcon from '@mui/icons-material/Remove';

import ScenarioIO from './ScenarioIO';
import React from 'react';

const SectionHeader = ({
    title,
    onAdd,
}: {
    title: string;
    onAdd: () => void;
}) => (
    <Box
        sx={{
            display: 'flex',
            justifyContent: 'space-between',
            alignItems: 'center',
        }}
    >
        <Typography variant="overline" sx={{ color: 'text.secondary' }}>
            {title}
        </Typography>
        <Tooltip title={`Add ${title.slice(0, -1)}`}>
            <IconButton
                size="small"
                onClick={(e) => {
                    e.stopPropagation();
                    onAdd();
                }}
            >
                <AddIcon fontSize="inherit" />
            </IconButton>
        </Tooltip>
    </Box>
);

export default function SceneTree() {
    const {
        globalParameters,
        pulses,
        timings,
        antennas,
        platforms,
        selectedItemId,
        selectItem,
        addPulse,
        addTiming,
        addAntenna,
        addPlatform,
        removeItem,
    } = useScenarioStore();

    const handleSelect = (
        _event: React.SyntheticEvent | null,
        nodeId: string | null
    ) => {
        const rootNodes = [
            'pulses-root',
            'timings-root',
            'antennas-root',
            'platforms-root',
        ];
        if (nodeId && rootNodes.includes(nodeId)) {
            return;
        }
        selectItem(nodeId);
    };

    return (
        <Box
            sx={{
                height: '100%',
                display: 'flex',
                flexDirection: 'column',
            }}
        >
            {/* 1. Fixed Header */}
            <Box
                sx={{
                    display: 'flex',
                    justifyContent: 'space-between',
                    alignItems: 'center',
                    flexShrink: 0,
                    px: 2,
                    pt: 2,
                    pb: 1,
                }}
            >
                <Typography variant="overline" sx={{ color: 'text.secondary' }}>
                    Scenario Explorer
                </Typography>
                <Box>
                    <ScenarioIO />
                </Box>
            </Box>
            <Divider sx={{ mx: 2, mb: 1 }} />

            {/* 2. Scrollable Content Area */}
            <Box
                sx={{
                    flexGrow: 1, // Takes up all remaining space
                    overflowY: 'auto', // Enables vertical scrolling
                    minHeight: 0, // Crucial for flexbox scrolling
                    px: 2, // Apply horizontal padding inside scroll area
                }}
            >
                <SimpleTreeView
                    selectedItems={selectedItemId}
                    onSelectedItemsChange={handleSelect}
                    slots={{
                        collapseIcon: ExpandMoreIcon,
                        expandIcon: ChevronRightIcon,
                    }}
                    sx={{
                        // Show remove button on hover
                        '& .MuiTreeItem-content .remove-button': {
                            visibility: 'hidden',
                        },
                        '& .MuiTreeItem-content:hover .remove-button': {
                            visibility: 'visible',
                        },
                        '& .MuiTreeItem-content': { py: 0.5 },
                    }}
                >
                    <TreeItem
                        itemId={globalParameters.id}
                        label={
                            <Box sx={{ display: 'flex', alignItems: 'center' }}>
                                <PublicIcon sx={{ mr: 1 }} fontSize="small" />
                                <Typography variant="body2">
                                    Global Parameters
                                </Typography>
                            </Box>
                        }
                    />
                    <TreeItem
                        itemId="pulses-root"
                        label={
                            <SectionHeader title="Pulses" onAdd={addPulse} />
                        }
                    >
                        {pulses.map((pulse) => (
                            <TreeItem
                                key={pulse.id}
                                itemId={pulse.id}
                                label={
                                    <Box
                                        sx={{
                                            display: 'flex',
                                            alignItems: 'center',
                                            width: '100%',
                                        }}
                                    >
                                        <WavesIcon
                                            sx={{ mr: 1 }}
                                            fontSize="small"
                                        />
                                        <Typography
                                            variant="body2"
                                            sx={{ flexGrow: 1 }}
                                        >
                                            {pulse.name}
                                        </Typography>
                                        <IconButton
                                            size="small"
                                            className="remove-button"
                                            onClick={(e) => {
                                                e.stopPropagation();
                                                removeItem(pulse.id);
                                            }}
                                        >
                                            <RemoveIcon fontSize="inherit" />
                                        </IconButton>
                                    </Box>
                                }
                            />
                        ))}
                    </TreeItem>
                    <TreeItem
                        itemId="timings-root"
                        label={
                            <SectionHeader title="Timings" onAdd={addTiming} />
                        }
                    >
                        {timings.map((timing) => (
                            <TreeItem
                                key={timing.id}
                                itemId={timing.id}
                                label={
                                    <Box
                                        sx={{
                                            display: 'flex',
                                            alignItems: 'center',
                                            width: '100%',
                                        }}
                                    >
                                        <TimerIcon
                                            sx={{ mr: 1 }}
                                            fontSize="small"
                                        />
                                        <Typography
                                            variant="body2"
                                            sx={{ flexGrow: 1 }}
                                        >
                                            {timing.name}
                                        </Typography>
                                        <IconButton
                                            size="small"
                                            className="remove-button"
                                            onClick={(e) => {
                                                e.stopPropagation();
                                                removeItem(timing.id);
                                            }}
                                        >
                                            <RemoveIcon fontSize="inherit" />
                                        </IconButton>
                                    </Box>
                                }
                            />
                        ))}
                    </TreeItem>
                    <TreeItem
                        itemId="antennas-root"
                        label={
                            <SectionHeader
                                title="Antennas"
                                onAdd={addAntenna}
                            />
                        }
                    >
                        {antennas.map((antenna) => (
                            <TreeItem
                                key={antenna.id}
                                itemId={antenna.id}
                                label={
                                    <Box
                                        sx={{
                                            display: 'flex',
                                            alignItems: 'center',
                                            width: '100%',
                                        }}
                                    >
                                        <SettingsInputAntennaIcon
                                            sx={{ mr: 1 }}
                                            fontSize="small"
                                        />
                                        <Typography
                                            variant="body2"
                                            sx={{ flexGrow: 1 }}
                                        >
                                            {antenna.name}
                                        </Typography>
                                        <IconButton
                                            size="small"
                                            className="remove-button"
                                            onClick={(e) => {
                                                e.stopPropagation();
                                                removeItem(antenna.id);
                                            }}
                                        >
                                            <RemoveIcon fontSize="inherit" />
                                        </IconButton>
                                    </Box>
                                }
                            />
                        ))}
                    </TreeItem>
                    <TreeItem
                        itemId="platforms-root"
                        label={
                            <SectionHeader
                                title="Platforms"
                                onAdd={addPlatform}
                            />
                        }
                    >
                        {platforms.map((platform) => (
                            <TreeItem
                                key={platform.id}
                                itemId={platform.id}
                                label={
                                    <Box
                                        sx={{
                                            display: 'flex',
                                            alignItems: 'center',
                                            width: '100%',
                                        }}
                                    >
                                        <FlightIcon
                                            sx={{ mr: 1 }}
                                            fontSize="small"
                                        />
                                        <Typography
                                            variant="body2"
                                            sx={{ flexGrow: 1 }}
                                        >
                                            {platform.name}
                                        </Typography>
                                        <IconButton
                                            size="small"
                                            className="remove-button"
                                            onClick={(e) => {
                                                e.stopPropagation();
                                                removeItem(platform.id);
                                            }}
                                        >
                                            <RemoveIcon fontSize="inherit" />
                                        </IconButton>
                                    </Box>
                                }
                            />
                        ))}
                    </TreeItem>
                </SimpleTreeView>
            </Box>
        </Box>
    );
}
