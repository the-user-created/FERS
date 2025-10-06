// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { IconButton, Tooltip } from '@mui/material';
import FileUploadIcon from '@mui/icons-material/FileUpload';
import FileDownloadIcon from '@mui/icons-material/FileDownload';
import { useScenarioStore } from '@/stores/scenarioStore';
import { invoke } from '@tauri-apps/api/core';
import { save, open } from '@tauri-apps/plugin-dialog';
import { writeTextFile } from '@tauri-apps/plugin-fs';

export default function ScenarioIO() {
    const { loadScenario } = useScenarioStore.getState();

    const handleExport = async () => {
        try {
            // First, ensure the backend has the latest UI state.
            // A more robust implementation might do this on every change,
            // but for an explicit export, syncing first is sufficient.
            await useScenarioStore.getState().syncBackend();

            // Now, get the canonical XML from the C++ core.
            const xmlContent = await invoke<string>('get_scenario_as_xml');

            const filePath = await save({
                title: 'Export Scenario',
                filters: [
                    {
                        name: 'FERS XML Scenario',
                        extensions: ['xml', 'fersxml'],
                    },
                ],
            });

            if (filePath) {
                await writeTextFile(filePath, xmlContent);
                console.log('Scenario exported successfully to:', filePath);
            }
        } catch (error) {
            console.error('Failed to export scenario:', error);
            // Here you could show an error dialog/snackbar
        }
    };

    const handleImport = async () => {
        try {
            const selectedPath = await open({
                title: 'Import Scenario',
                multiple: false,
                filters: [
                    {
                        name: 'FERS XML Scenario',
                        extensions: ['xml', 'fersxml'],
                    },
                ],
            });

            if (typeof selectedPath === 'string') {
                // Load the XML file into the C++ core
                await invoke('load_scenario_from_xml_file', {
                    filepath: selectedPath,
                });

                // Fetch the new state as JSON from the C++ core
                const jsonState = await invoke<string>('get_scenario_as_json');
                const scenarioData = JSON.parse(jsonState);

                // Update the UI's Zustand store with the new state
                loadScenario(scenarioData);

                console.log(
                    'Scenario imported and synchronized successfully from:',
                    selectedPath
                );
            }
        } catch (error) {
            console.error('Failed to import scenario:', error);
        }
    };

    return (
        <>
            <Tooltip title="Import Scenario (XML)">
                <IconButton size="small" onClick={handleImport}>
                    <FileUploadIcon fontSize="inherit" />
                </IconButton>
            </Tooltip>
            <Tooltip title="Export Scenario (XML)">
                <IconButton size="small" onClick={handleExport}>
                    <FileDownloadIcon fontSize="inherit" />
                </IconButton>
            </Tooltip>
        </>
    );
}
