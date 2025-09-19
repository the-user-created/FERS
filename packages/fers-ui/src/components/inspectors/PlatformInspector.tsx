// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import {
    Box,
    Button,
    FormControl,
    IconButton,
    InputLabel,
    MenuItem,
    Select,
    TextField,
} from '@mui/material';
import {
    useScenarioStore,
    Platform,
    PlatformComponent,
} from '@/stores/scenarioStore';
import { Section, NumberField } from './InspectorControls';
import DeleteIcon from '@mui/icons-material/Delete';
import { v4 as uuidv4 } from 'uuid';
import { PlatformComponentInspector } from './PlatformComponentInspector';

export function PlatformInspector({ item }: { item: Platform }) {
    const {
        updateItem,
        addPositionWaypoint,
        removePositionWaypoint,
        addRotationWaypoint,
        removeRotationWaypoint,
        setPlatformComponentType,
    } = useScenarioStore.getState();
    const handleChange = (path: string, value: unknown) =>
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
                                e.target.value as PlatformComponent['type']
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
