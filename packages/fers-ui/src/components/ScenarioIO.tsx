// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { IconButton, Tooltip } from '@mui/material';
import FileUploadIcon from '@mui/icons-material/FileUpload';
import FileDownloadIcon from '@mui/icons-material/FileDownload';
import { useScenarioStore, type ScenarioData } from '@/stores/scenarioStore';
import { invoke } from '@tauri-apps/api/core';
import { save, open } from '@tauri-apps/plugin-dialog';
import { writeTextFile, readTextFile } from '@tauri-apps/plugin-fs';

export default function ScenarioIO() {
    const handleExport = async () => {
        try {
            // Get the latest state directly inside the handler
            const currentState = useScenarioStore.getState();
            const scenarioState: ScenarioData = {
                globalParameters: currentState.globalParameters,
                pulses: currentState.pulses,
                timings: currentState.timings,
                antennas: currentState.antennas,
                platforms: currentState.platforms,
            };

            const xmlContent = await invoke<string>('generate_xml', {
                scenario: scenarioState,
            });

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
                const xmlContent = await readTextFile(selectedPath);
                const newStateJson = await invoke<string>('parse_xml', {
                    xmlContent,
                });
                const newState: ScenarioData = JSON.parse(newStateJson);

                console.log(
                    '[DEBUG] Parsed state from Rust:',
                    JSON.stringify(newState, null, 2)
                );

                useScenarioStore.getState().loadScenario(newState);
                console.log(
                    'Scenario imported successfully from:',
                    selectedPath
                );
            }
        } catch (error) {
            console.error('Failed to import scenario:', error);
            // Here you could show an error dialog/snackbar
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
