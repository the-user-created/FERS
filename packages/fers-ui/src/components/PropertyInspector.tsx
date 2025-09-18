// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import {
    Box,
    Typography,
    Divider,
    TextField,
    FormControl,
    InputLabel,
    Select,
    MenuItem,
    FormGroup,
    FormControlLabel,
    Checkbox,
    Button,
    IconButton,
    Accordion,
    AccordionSummary,
    AccordionDetails,
} from '@mui/material';
import {
    useScenarioStore,
    findItemInStore,
    Platform,
    Pulse,
    Timing,
    Antenna,
    GlobalParameters,
    PlatformComponent,
} from '@/stores/scenarioStore';
import ExpandMoreIcon from '@mui/icons-material/ExpandMore';
import DeleteIcon from '@mui/icons-material/Delete';
import React from 'react';
import { open } from '@tauri-apps/plugin-dialog';
import { v4 as uuidv4 } from 'uuid';

// --- Helper Components ---
const Section = ({
    title,
    children,
}: {
    title: string;
    children: React.ReactNode;
}) => (
    <Accordion
        defaultExpanded
        sx={{ backgroundImage: 'none', boxShadow: 'none' }}
    >
        <AccordionSummary expandIcon={<ExpandMoreIcon />} sx={{ p: 0 }}>
            <Typography variant="subtitle2">{title}</Typography>
        </AccordionSummary>
        <AccordionDetails
            sx={{ p: 0, display: 'flex', flexDirection: 'column', gap: 2 }}
        >
            {children}
        </AccordionDetails>
    </Accordion>
);

const NumberField = ({
    label,
    value,
    onChange,
}: {
    label: string;
    value: number | null;
    onChange: (val: number | null) => void;
}) => (
    <TextField
        label={label}
        type="number"
        variant="outlined"
        size="small"
        fullWidth
        value={value ?? ''}
        onChange={(e) =>
            onChange(e.target.value === '' ? null : parseFloat(e.target.value))
        }
    />
);

const FileInput = ({
    label,
    value,
    onChange,
    filters,
}: {
    label: string;
    value?: string;
    onChange: (val: string) => void;
    filters: { name: string; extensions: string[] }[];
}) => {
    const handleOpenFile = async () => {
        try {
            const selected = await open({
                multiple: false,
                filters,
            });
            if (typeof selected === 'string') {
                onChange(selected);
            }
        } catch (err) {
            console.error('Error opening file dialog:', err);
        }
    };

    return (
        <Box>
            <Typography variant="caption" color="text.secondary">
                {label}
            </Typography>
            <TextField
                variant="outlined"
                size="small"
                fullWidth
                value={value?.split(/[/\\]/).pop() ?? 'No file selected.'}
                onClick={handleOpenFile}
                sx={{ cursor: 'pointer' }}
                slotProps={{
                    input: {
                        readOnly: true,
                        style: { cursor: 'pointer' },
                    },
                }}
            />
        </Box>
    );
};

// --- Inspector Components ---

function GlobalParametersInspector({ item }: { item: GlobalParameters }) {
    const { updateItem } = useScenarioStore.getState();
    const handleChange = (path: string, value: any) =>
        updateItem(item.id, path, value);

    return (
        <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
            <TextField
                label="Simulation Name"
                variant="outlined"
                size="small"
                fullWidth
                value={item.simulation_name}
                onChange={(e) =>
                    handleChange('simulation_name', e.target.value)
                }
            />
            <NumberField
                label="Start Time (s)"
                value={item.start}
                onChange={(v) => handleChange('start', v)}
            />
            <NumberField
                label="End Time (s)"
                value={item.end}
                onChange={(v) => handleChange('end', v)}
            />
            <NumberField
                label="Sampling Rate (Hz)"
                value={item.rate}
                onChange={(v) => handleChange('rate', v)}
            />
            <NumberField
                label="Speed of Light (m/s)"
                value={item.c}
                onChange={(v) => handleChange('c', v)}
            />
            <NumberField
                label="Random Seed"
                value={item.random_seed}
                onChange={(v) => handleChange('random_seed', v)}
            />
            <NumberField
                label="ADC Bits"
                value={item.adc_bits}
                onChange={(v) => handleChange('adc_bits', v)}
            />
            <NumberField
                label="Oversample Ratio"
                value={item.oversample_ratio}
                onChange={(v) => handleChange('oversample_ratio', v)}
            />
            <FormControl component="fieldset">
                <Typography variant="body2">Export Options</Typography>
                <FormGroup row>
                    <FormControlLabel
                        control={
                            <Checkbox
                                checked={item.export.xml}
                                onChange={(e) =>
                                    handleChange('export.xml', e.target.checked)
                                }
                            />
                        }
                        label="XML"
                    />
                    <FormControlLabel
                        control={
                            <Checkbox
                                checked={item.export.csv}
                                onChange={(e) =>
                                    handleChange('export.csv', e.target.checked)
                                }
                            />
                        }
                        label="CSV"
                    />
                    <FormControlLabel
                        control={
                            <Checkbox
                                checked={item.export.binary}
                                onChange={(e) =>
                                    handleChange(
                                        'export.binary',
                                        e.target.checked
                                    )
                                }
                            />
                        }
                        label="H5 (Binary)"
                    />
                </FormGroup>
            </FormControl>
        </Box>
    );
}

function PulseInspector({ item }: { item: Pulse }) {
    const { updateItem } = useScenarioStore.getState();
    const handleChange = (path: string, value: any) =>
        updateItem(item.id, path, value);

    return (
        <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
            <TextField
                label="Name"
                variant="outlined"
                size="small"
                fullWidth
                value={item.name}
                onChange={(e) => handleChange('name', e.target.value)}
            />
            <FormControl fullWidth size="small">
                <InputLabel>Type</InputLabel>
                <Select
                    label="Type"
                    value={item.pulseType}
                    onChange={(e) => handleChange('pulseType', e.target.value)}
                >
                    <MenuItem value="file">File</MenuItem>
                    <MenuItem value="continuous">Continuous</MenuItem>
                    <MenuItem value="cw">CW</MenuItem>
                </Select>
            </FormControl>
            <NumberField
                label="Power (W)"
                value={item.power}
                onChange={(v) => handleChange('power', v)}
            />
            <NumberField
                label="Carrier Frequency (Hz)"
                value={item.carrier}
                onChange={(v) => handleChange('carrier', v)}
            />
            {item.pulseType === 'file' && (
                <FileInput
                    label="Waveform File (.csv, .h5)"
                    value={item.filename}
                    onChange={(v) => handleChange('filename', v)}
                    filters={[
                        { name: 'Waveform', extensions: ['csv', 'h5'] },
                        { name: 'All Files', extensions: ['*'] },
                    ]}
                />
            )}
        </Box>
    );
}

function TimingInspector({ item }: { item: Timing }) {
    const { updateItem } = useScenarioStore.getState();
    const handleChange = (path: string, value: any) =>
        updateItem(item.id, path, value);

    return (
        <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
            <TextField
                label="Name"
                variant="outlined"
                size="small"
                fullWidth
                value={item.name}
                onChange={(e) => handleChange('name', e.target.value)}
            />
            <NumberField
                label="Frequency (Hz)"
                value={item.frequency}
                onChange={(v) => handleChange('frequency', v)}
            />
        </Box>
    );
}

function AntennaInspector({ item }: { item: Antenna }) {
    const { updateItem } = useScenarioStore.getState();
    const handleChange = (path: string, value: any) =>
        updateItem(item.id, path, value);

    return (
        <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
            <TextField
                label="Name"
                variant="outlined"
                size="small"
                fullWidth
                value={item.name}
                onChange={(e) => handleChange('name', e.target.value)}
            />
            <FormControl fullWidth size="small">
                <InputLabel>Pattern</InputLabel>
                <Select
                    label="Pattern"
                    value={item.pattern}
                    onChange={(e) => handleChange('pattern', e.target.value)}
                >
                    <MenuItem value="sinc">Sinc</MenuItem>
                    <MenuItem value="gaussian">Gaussian</MenuItem>
                    <MenuItem value="squarehorn">Square Horn</MenuItem>
                    <MenuItem value="parabolic">Parabolic</MenuItem>
                    <MenuItem value="xml">XML</MenuItem>
                    <MenuItem value="file">File (H5)</MenuItem>
                </Select>
            </FormControl>
            {(item.pattern === 'xml' || item.pattern === 'file') && (
                <FileInput
                    label="Pattern File"
                    value={item.filename}
                    onChange={(v) => handleChange('filename', v)}
                    filters={[
                        {
                            name: 'Antenna Pattern',
                            extensions:
                                item.pattern === 'xml' ? ['xml'] : ['h5'],
                        },
                        { name: 'All Files', extensions: ['*'] },
                    ]}
                />
            )}
        </Box>
    );
}

function PlatformComponentInspector({
    component,
    platformId,
}: {
    component: PlatformComponent;
    platformId: string;
}) {
    const { updateItem, pulses, timings, antennas } =
        useScenarioStore.getState();
    const handleChange = (path: string, value: any) =>
        updateItem(platformId, `component.${path}`, value);

    const renderCommonFields = (c: {
        name: string;
        radarType: 'pulsed' | 'continuous' | 'cw';
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
                    <MenuItem value="continuous">Continuous</MenuItem>
                    <MenuItem value="cw">CW</MenuItem>
                </Select>
            </FormControl>
            <NumberField
                label="PRF (Hz)"
                value={c.prf}
                onChange={(v) => handleChange('prf', v)}
            />
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

function PlatformInspector({ item }: { item: Platform }) {
    const {
        updateItem,
        addPositionWaypoint,
        removePositionWaypoint,
        addRotationWaypoint,
        removeRotationWaypoint,
        setPlatformComponentType,
    } = useScenarioStore.getState();
    const handleChange = (path: string, value: any) =>
        updateItem(item.id, path, value);
    const allowMultiplePosWaypoints =
        item.motionPath.interpolation !== 'static';

    const handleRotationTypeChange = (newType: 'fixed' | 'path') => {
        if (newType === 'fixed' && item.rotation.type !== 'fixed') {
            handleChange('rotation', {
                type: 'fixed',
                startAzimuth: 0,
                startElevation: 0,
                azimuthRate: 0,
                elevationRate: 0,
            });
        } else if (newType === 'path' && item.rotation.type !== 'path') {
            handleChange('rotation', {
                type: 'path',
                interpolation: 'static',
                waypoints: [
                    { id: uuidv4(), azimuth: 0, elevation: 0, time: 0 },
                ],
            });
        }
    };

    return (
        <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
            <TextField
                label="Name"
                variant="outlined"
                size="small"
                fullWidth
                value={item.name}
                onChange={(e) => handleChange('name', e.target.value)}
            />

            <Section title="Motion Path">
                <FormControl fullWidth size="small">
                    <InputLabel>Interpolation</InputLabel>
                    <Select
                        label="Interpolation"
                        value={item.motionPath.interpolation}
                        onChange={(e) =>
                            handleChange(
                                'motionPath.interpolation',
                                e.target.value
                            )
                        }
                    >
                        <MenuItem value="static">Static</MenuItem>
                        <MenuItem value="linear">Linear</MenuItem>
                        <MenuItem value="cubic">Cubic</MenuItem>
                    </Select>
                </FormControl>
                {item.motionPath.waypoints
                    .slice(
                        0,
                        allowMultiplePosWaypoints
                            ? item.motionPath.waypoints.length
                            : 1
                    )
                    .map((wp, i) => (
                        <Box
                            key={wp.id}
                            sx={{
                                display: 'flex',
                                gap: 1,
                                alignItems: 'center',
                            }}
                        >
                            <NumberField
                                label="X"
                                value={wp.x}
                                onChange={(v) =>
                                    handleChange(
                                        `motionPath.waypoints.${i}.x`,
                                        v
                                    )
                                }
                            />
                            <NumberField
                                label="Y"
                                value={wp.y}
                                onChange={(v) =>
                                    handleChange(
                                        `motionPath.waypoints.${i}.y`,
                                        v
                                    )
                                }
                            />
                            <NumberField
                                label="Alt"
                                value={wp.altitude}
                                onChange={(v) =>
                                    handleChange(
                                        `motionPath.waypoints.${i}.altitude`,
                                        v
                                    )
                                }
                            />
                            <NumberField
                                label="T"
                                value={wp.time}
                                onChange={(v) =>
                                    handleChange(
                                        `motionPath.waypoints.${i}.time`,
                                        v
                                    )
                                }
                            />
                            {allowMultiplePosWaypoints && (
                                <IconButton
                                    size="small"
                                    onClick={() =>
                                        removePositionWaypoint(item.id, wp.id)
                                    }
                                >
                                    <DeleteIcon />
                                </IconButton>
                            )}
                        </Box>
                    ))}
                {allowMultiplePosWaypoints && (
                    <Button
                        onClick={() => addPositionWaypoint(item.id)}
                        size="small"
                    >
                        Add Waypoint
                    </Button>
                )}
            </Section>

            <Section title="Rotation">
                <FormControl fullWidth size="small">
                    <InputLabel>Rotation Type</InputLabel>
                    <Select
                        label="Rotation Type"
                        value={item.rotation.type}
                        onChange={(e) =>
                            handleRotationTypeChange(
                                e.target.value as 'fixed' | 'path'
                            )
                        }
                    >
                        <MenuItem value="fixed">Fixed Rate</MenuItem>
                        <MenuItem value="path">Waypoint Path</MenuItem>
                    </Select>
                </FormControl>
                {item.rotation.type === 'fixed' && (
                    <>
                        <NumberField
                            label="Start Azimuth (deg)"
                            value={item.rotation.startAzimuth}
                            onChange={(v) =>
                                handleChange('rotation.startAzimuth', v)
                            }
                        />
                        <NumberField
                            label="Start Elevation (deg)"
                            value={item.rotation.startElevation}
                            onChange={(v) =>
                                handleChange('rotation.startElevation', v)
                            }
                        />
                        <NumberField
                            label="Azimuth Rate (deg/s)"
                            value={item.rotation.azimuthRate}
                            onChange={(v) =>
                                handleChange('rotation.azimuthRate', v)
                            }
                        />
                        <NumberField
                            label="Elevation Rate (deg/s)"
                            value={item.rotation.elevationRate}
                            onChange={(v) =>
                                handleChange('rotation.elevationRate', v)
                            }
                        />
                    </>
                )}
                {item.rotation.type === 'path' &&
                    (() => {
                        const rotation = item.rotation;
                        const allowMultipleWaypoints =
                            rotation.interpolation !== 'static';
                        return (
                            <>
                                <FormControl fullWidth size="small">
                                    <InputLabel>Interpolation</InputLabel>
                                    <Select
                                        label="Interpolation"
                                        value={rotation.interpolation}
                                        onChange={(e) =>
                                            handleChange(
                                                'rotation.interpolation',
                                                e.target.value
                                            )
                                        }
                                    >
                                        <MenuItem value="static">
                                            Static
                                        </MenuItem>
                                        <MenuItem value="linear">
                                            Linear
                                        </MenuItem>
                                        <MenuItem value="cubic">Cubic</MenuItem>
                                    </Select>
                                </FormControl>
                                {rotation.waypoints
                                    .slice(
                                        0,
                                        allowMultipleWaypoints
                                            ? rotation.waypoints.length
                                            : 1
                                    )
                                    .map((wp, i) => (
                                        <Box
                                            key={wp.id}
                                            sx={{
                                                display: 'flex',
                                                gap: 1,
                                                alignItems: 'center',
                                            }}
                                        >
                                            <NumberField
                                                label="Az (deg)"
                                                value={wp.azimuth}
                                                onChange={(v) =>
                                                    handleChange(
                                                        `rotation.waypoints.${i}.azimuth`,
                                                        v
                                                    )
                                                }
                                            />
                                            <NumberField
                                                label="El (deg)"
                                                value={wp.elevation}
                                                onChange={(v) =>
                                                    handleChange(
                                                        `rotation.waypoints.${i}.elevation`,
                                                        v
                                                    )
                                                }
                                            />
                                            <NumberField
                                                label="Time (s)"
                                                value={wp.time}
                                                onChange={(v) =>
                                                    handleChange(
                                                        `rotation.waypoints.${i}.time`,
                                                        v
                                                    )
                                                }
                                            />
                                            {allowMultipleWaypoints && (
                                                <IconButton
                                                    size="small"
                                                    onClick={() =>
                                                        removeRotationWaypoint(
                                                            item.id,
                                                            wp.id
                                                        )
                                                    }
                                                >
                                                    <DeleteIcon />
                                                </IconButton>
                                            )}
                                        </Box>
                                    ))}
                                {allowMultipleWaypoints && (
                                    <Button
                                        onClick={() =>
                                            addRotationWaypoint(item.id)
                                        }
                                        size="small"
                                    >
                                        Add Waypoint
                                    </Button>
                                )}
                            </>
                        );
                    })()}
            </Section>

            <Section title="Component">
                <FormControl fullWidth size="small">
                    <InputLabel>Component Type</InputLabel>
                    <Select
                        label="Component Type"
                        value={item.component.type}
                        onChange={(e) =>
                            setPlatformComponentType(
                                item.id,
                                e.target.value as any
                            )
                        }
                    >
                        <MenuItem value="none">None</MenuItem>
                        <MenuItem value="monostatic">Monostatic Radar</MenuItem>
                        <MenuItem value="transmitter">Transmitter</MenuItem>
                        <MenuItem value="receiver">Receiver</MenuItem>
                        <MenuItem value="target">Target</MenuItem>
                    </Select>
                </FormControl>
                <PlatformComponentInspector
                    component={item.component}
                    platformId={item.id}
                />
            </Section>
        </Box>
    );
}

function InspectorContent() {
    const { selectedItemId } = useScenarioStore();
    const selectedItem = findItemInStore(selectedItemId);

    if (!selectedItem) {
        return (
            <Typography sx={{ p: 2 }} color="text.secondary">
                Select an item to see its properties.
            </Typography>
        );
    }

    let InspectorComponent;
    switch (selectedItem.type) {
        case 'GlobalParameters':
            InspectorComponent = (
                <GlobalParametersInspector item={selectedItem} />
            );
            break;
        case 'Pulse':
            InspectorComponent = <PulseInspector item={selectedItem} />;
            break;
        case 'Timing':
            InspectorComponent = <TimingInspector item={selectedItem} />;
            break;
        case 'Antenna':
            InspectorComponent = <AntennaInspector item={selectedItem} />;
            break;
        case 'Platform':
            InspectorComponent = <PlatformInspector item={selectedItem} />;
            break;
        default:
            return null;
    }

    return (
        <Box sx={{ p: 1 }}>
            <Typography variant="overline" color="text.secondary">
                {selectedItem.type}
            </Typography>
            <Divider sx={{ my: 1 }} />
            {InspectorComponent}
        </Box>
    );
}

export default function PropertyInspector() {
    return (
        <Box>
            <Typography variant="h6" gutterBottom sx={{ px: 1, pt: 0.5 }}>
                Properties
            </Typography>
            <InspectorContent />
        </Box>
    );
}
