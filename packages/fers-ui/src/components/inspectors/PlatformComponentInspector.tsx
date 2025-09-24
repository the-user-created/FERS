// ./packages/fers-ui/src/components/inspectors/PlatformComponentInspector.tsx
// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import {
    Checkbox,
    FormControl,
    FormControlLabel,
    InputLabel,
    MenuItem,
    Select,
    TextField,
    Typography,
} from '@mui/material';
import { useScenarioStore, PlatformComponent } from '@/stores/scenarioStore';
import { NumberField, FileInput } from './InspectorControls';

export function PlatformComponentInspector({
    component,
    platformId,
}: {
    component: PlatformComponent;
    platformId: string;
}) {
    const { updateItem, pulses, timings, antennas } =
        useScenarioStore.getState();
    const handleChange = (path: string, value: unknown) =>
        updateItem(platformId, `component.${path}`, value);

    const renderCommonFields = (c: {
        name: string;
        radarType: 'pulsed' | 'cw';
        prf: number;
        antennaId: string | null;
        pulseId: string | null;
        timingId: string | null;
    }) => (
        <>
            <TextField
                label="Component Name"
                size="small"
                value={c.name}
                onChange={(e) => handleChange('name', e.target.value)}
            />
            <FormControl fullWidth size="small">
                <InputLabel>Type</InputLabel>
                <Select
                    label="Type"
                    value={c.radarType}
                    onChange={(e) => handleChange('radarType', e.target.value)}
                >
                    <MenuItem value="pulsed">Pulsed</MenuItem>
                    <MenuItem value="cw">CW</MenuItem>
                </Select>
            </FormControl>
            {c.radarType === 'pulsed' && (
                <NumberField
                    label="PRF (Hz)"
                    value={c.prf}
                    onChange={(v) => handleChange('prf', v)}
                />
            )}
            <FormControl fullWidth size="small">
                <InputLabel>Antenna</InputLabel>
                <Select
                    label="Antenna"
                    value={c.antennaId ?? ''}
                    onChange={(e) => handleChange('antennaId', e.target.value)}
                >
                    {antennas.map((a) => (
                        <MenuItem key={a.id} value={a.id}>
                            {a.name}
                        </MenuItem>
                    ))}
                </Select>
            </FormControl>
            <FormControl fullWidth size="small">
                <InputLabel>Pulse</InputLabel>
                <Select
                    label="Pulse"
                    value={c.pulseId ?? ''}
                    onChange={(e) => handleChange('pulseId', e.target.value)}
                >
                    {pulses.map((p) => (
                        <MenuItem key={p.id} value={p.id}>
                            {p.name}
                        </MenuItem>
                    ))}
                </Select>
            </FormControl>
            <FormControl fullWidth size="small">
                <InputLabel>Timing Source</InputLabel>
                <Select
                    label="Timing Source"
                    value={c.timingId ?? ''}
                    onChange={(e) => handleChange('timingId', e.target.value)}
                >
                    {timings.map((t) => (
                        <MenuItem key={t.id} value={t.id}>
                            {t.name}
                        </MenuItem>
                    ))}
                </Select>
            </FormControl>
        </>
    );

    switch (component.type) {
        case 'monostatic':
            return (
                <>
                    {renderCommonFields(component)}
                    <NumberField
                        label="Window Skip (s)"
                        value={component.window_skip}
                        onChange={(v) => handleChange('window_skip', v)}
                    />
                    <NumberField
                        label="Window Length (s)"
                        value={component.window_length}
                        onChange={(v) => handleChange('window_length', v)}
                    />
                    <NumberField
                        label="Noise Temperature (K)"
                        value={component.noiseTemperature}
                        onChange={(v) => handleChange('noiseTemperature', v)}
                    />
                    <FormControlLabel
                        control={
                            <Checkbox
                                checked={component.noDirectPaths}
                                onChange={(e) =>
                                    handleChange(
                                        'noDirectPaths',
                                        e.target.checked
                                    )
                                }
                            />
                        }
                        label="Ignore Direct Paths"
                    />
                    <FormControlLabel
                        control={
                            <Checkbox
                                checked={component.noPropagationLoss}
                                onChange={(e) =>
                                    handleChange(
                                        'noPropagationLoss',
                                        e.target.checked
                                    )
                                }
                            />
                        }
                        label="Ignore Propagation Loss"
                    />
                </>
            );
        case 'transmitter':
            return renderCommonFields(component);
        case 'receiver':
            return (
                <>
                    <TextField
                        label="Component Name"
                        size="small"
                        value={component.name}
                        onChange={(e) => handleChange('name', e.target.value)}
                    />
                    <NumberField
                        label="Window Skip (s)"
                        value={component.window_skip}
                        onChange={(v) => handleChange('window_skip', v)}
                    />
                    <NumberField
                        label="Window Length (s)"
                        value={component.window_length}
                        onChange={(v) => handleChange('window_length', v)}
                    />
                    <NumberField
                        label="PRF (Hz)"
                        value={component.prf}
                        onChange={(v) => handleChange('prf', v)}
                    />
                    <FormControl fullWidth size="small">
                        <InputLabel>Antenna</InputLabel>
                        <Select
                            label="Antenna"
                            value={component.antennaId ?? ''}
                            onChange={(e) =>
                                handleChange('antennaId', e.target.value)
                            }
                        >
                            {antennas.map((a) => (
                                <MenuItem key={a.id} value={a.id}>
                                    {a.name}
                                </MenuItem>
                            ))}
                        </Select>
                    </FormControl>
                    <FormControl fullWidth size="small">
                        <InputLabel>Timing Source</InputLabel>
                        <Select
                            label="Timing Source"
                            value={component.timingId ?? ''}
                            onChange={(e) =>
                                handleChange('timingId', e.target.value)
                            }
                        >
                            {timings.map((t) => (
                                <MenuItem key={t.id} value={t.id}>
                                    {t.name}
                                </MenuItem>
                            ))}
                        </Select>
                    </FormControl>
                    <NumberField
                        label="Noise Temperature (K)"
                        value={component.noiseTemperature}
                        onChange={(v) => handleChange('noiseTemperature', v)}
                    />
                    <FormControlLabel
                        control={
                            <Checkbox
                                checked={component.noDirectPaths}
                                onChange={(e) =>
                                    handleChange(
                                        'noDirectPaths',
                                        e.target.checked
                                    )
                                }
                            />
                        }
                        label="Ignore Direct Paths"
                    />
                    <FormControlLabel
                        control={
                            <Checkbox
                                checked={component.noPropagationLoss}
                                onChange={(e) =>
                                    handleChange(
                                        'noPropagationLoss',
                                        e.target.checked
                                    )
                                }
                            />
                        }
                        label="Ignore Propagation Loss"
                    />
                </>
            );
        case 'target':
            return (
                <>
                    <TextField
                        label="Component Name"
                        size="small"
                        value={component.name}
                        onChange={(e) => handleChange('name', e.target.value)}
                    />
                    <FormControl fullWidth size="small">
                        <InputLabel>RCS Type</InputLabel>
                        <Select
                            label="RCS Type"
                            value={component.rcs_type}
                            onChange={(e) =>
                                handleChange('rcs_type', e.target.value)
                            }
                        >
                            <MenuItem value="isotropic">Isotropic</MenuItem>
                            <MenuItem value="file">File</MenuItem>
                        </Select>
                    </FormControl>
                    {component.rcs_type === 'isotropic' && (
                        <NumberField
                            label="RCS Value (m^2)"
                            value={component.rcs_value ?? 0}
                            onChange={(v) => handleChange('rcs_value', v)}
                        />
                    )}
                    {component.rcs_type === 'file' && (
                        <FileInput
                            label="RCS File"
                            value={component.rcs_filename}
                            onChange={(v) => handleChange('rcs_filename', v)}
                            filters={[{ name: 'RCS Data', extensions: ['*'] }]}
                        />
                    )}

                    <FormControl fullWidth size="small">
                        <InputLabel>RCS Model</InputLabel>
                        <Select
                            label="RCS Model"
                            value={component.rcs_model}
                            onChange={(e) =>
                                handleChange('rcs_model', e.target.value)
                            }
                        >
                            <MenuItem value="constant">Constant</MenuItem>
                            <MenuItem value="chisquare">Chi-Square</MenuItem>
                            <MenuItem value="gamma">Gamma</MenuItem>
                        </Select>
                    </FormControl>
                    {(component.rcs_model === 'chisquare' ||
                        component.rcs_model === 'gamma') && (
                        <NumberField
                            label="K Value"
                            value={component.rcs_k ?? 0}
                            onChange={(v) => handleChange('rcs_k', v)}
                        />
                    )}
                </>
            );
        default:
            return (
                <Typography color="text.secondary">
                    No component assigned.
                </Typography>
            );
    }
}
