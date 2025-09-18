// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { SimpleTreeView } from '@mui/x-tree-view/SimpleTreeView';
import { TreeItem } from '@mui/x-tree-view/TreeItem';

import ExpandMoreIcon from '@mui/icons-material/ExpandMore';
import ChevronRightIcon from '@mui/icons-material/ChevronRight';
import MemoryIcon from '@mui/icons-material/Memory';
import AdjustIcon from '@mui/icons-material/Adjust';
import SettingsInputAntennaIcon from '@mui/icons-material/SettingsInputAntenna';
import { Box, Typography } from '@mui/material';

// Helper component to render a label with an icon
const TreeItemLabel = ({
    Icon,
    text,
}: {
    Icon?: React.ElementType;
    text: string;
}) => (
    <Box sx={{ display: 'flex', alignItems: 'center', p: 0.5, py: 0 }}>
        {Icon && (
            <Box
                component={Icon}
                color="inherit"
                sx={{ mr: 1, fontSize: '1.25rem' }}
            />
        )}
        <Typography variant="body2" sx={{ fontWeight: 'inherit', flexGrow: 1 }}>
            {text}
        </Typography>
    </Box>
);

export default function SceneTree() {
    return (
        <Box>
            <Typography
                variant="overline"
                sx={{ px: 2, pt: 1, color: 'text.secondary' }}
            >
                Scenario
            </Typography>
            <SimpleTreeView
                defaultExpandedItems={['platforms']}
                slots={{
                    collapseIcon: ExpandMoreIcon,
                    expandIcon: ChevronRightIcon,
                }}
                sx={{ flexGrow: 1, maxWidth: 400, overflowY: 'auto' }}
            >
                <TreeItem itemId="global" label="Global Parameters">
                    <TreeItem itemId="origin" label="Origin: UCT" />
                    <TreeItem itemId="frame" label="Frame: ENU" />
                </TreeItem>
                <TreeItem itemId="platforms" label="Platforms">
                    <TreeItem itemId="awacs" label="AWACS">
                        <TreeItem
                            itemId="radar"
                            label={
                                <TreeItemLabel
                                    Icon={SettingsInputAntennaIcon}
                                    text="Radar: AN/APY-2"
                                />
                            }
                        />
                        <TreeItem itemId="awacs-motion" label="Motion Path" />
                    </TreeItem>
                    <TreeItem
                        itemId="f16"
                        label={<TreeItemLabel Icon={AdjustIcon} text="F-16" />}
                    />
                    <TreeItem
                        itemId="ground-station"
                        label={
                            <TreeItemLabel
                                Icon={MemoryIcon}
                                text="Ground Station"
                            />
                        }
                    />
                </TreeItem>
            </SimpleTreeView>
        </Box>
    );
}
