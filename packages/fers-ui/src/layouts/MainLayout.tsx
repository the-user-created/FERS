// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useState } from 'react';
import { Box } from '@mui/material';
import AppRail from '@/components/AppRail';
import { ScenarioView } from '@/views/ScenarioView';
import { AssetLibraryView } from '@/views/AssetLibraryView';
import { SimulationView } from '@/views/SimulationView';
import { ResultsView } from '@/views/ResultsView';
import SettingsDialog from '@/components/SettingsDialog';

const views: { [key: string]: React.ComponentType } = {
    scenario: ScenarioView,
    assets: AssetLibraryView,
    simulation: SimulationView,
    results: ResultsView,
};

export function MainLayout() {
    const [activeView, setActiveView] = useState('scenario');
    const [settingsOpen, setSettingsOpen] = useState(false);

    const ActiveViewComponent = views[activeView] || ScenarioView;

    return (
        <Box
            sx={{
                display: 'flex',
                height: '100vh',
                width: '100vw',
                overflow: 'hidden',
            }}
        >
            <AppRail
                activeView={activeView}
                onViewChange={setActiveView}
                onSettingsClick={() => setSettingsOpen(true)}
            />
            <Box
                component="main"
                sx={{ flexGrow: 1, minWidth: 0, height: '100%' }}
            >
                <ActiveViewComponent />
            </Box>
            <SettingsDialog
                open={settingsOpen}
                onClose={() => setSettingsOpen(false)}
            />
        </Box>
    );
}
