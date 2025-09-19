// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { SimpleTreeView, TreeItem } from '@mui/x-tree-view';
import { useScenarioStore, findItemInStore } from '@/stores/scenarioStore';
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
            <IconButton size="small" onClick={onAdd}>
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
        removeSelectedItem,
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

    const selectedItem = findItemInStore(selectedItemId);
    const canRemove = selectedItem && selectedItem.type !== 'GlobalParameters';

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
                <Tooltip title="Remove Selected Item">
                    <span>
                        <IconButton
                            size="small"
                            onClick={removeSelectedItem}
                            disabled={!canRemove}
                        >
                            <RemoveIcon />
                        </IconButton>
                    </span>
                </Tooltip>
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
                                        }}
                                    >
                                        <WavesIcon
                                            sx={{ mr: 1 }}
                                            fontSize="small"
                                        />
                                        <Typography variant="body2">
                                            {pulse.name}
                                        </Typography>
                                    </Box>
                                }
                            />
                        ))}
                    </TreeItem>
                    {/* ... other TreeItems ... */}
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
                                        }}
                                    >
                                        <TimerIcon
                                            sx={{ mr: 1 }}
                                            fontSize="small"
                                        />
                                        <Typography variant="body2">
                                            {timing.name}
                                        </Typography>
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
                                        }}
                                    >
                                        <SettingsInputAntennaIcon
                                            sx={{ mr: 1 }}
                                            fontSize="small"
                                        />
                                        <Typography variant="body2">
                                            {antenna.name}
                                        </Typography>
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
                                        }}
                                    >
                                        <FlightIcon
                                            sx={{ mr: 1 }}
                                            fontSize="small"
                                        />
                                        <Typography variant="body2">
                                            {platform.name}
                                        </Typography>
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
