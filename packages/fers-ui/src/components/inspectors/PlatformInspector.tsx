// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { useState } from 'react';
import {
    Box,
    Button,
    Dialog,
    DialogActions,
    DialogContent,
    DialogTitle,
    FormControl,
    IconButton,
    InputLabel,
    MenuItem,
    Select,
    TextField,
    Typography,
} from '@mui/material';
import {
    useScenarioStore,
    Platform,
    PlatformComponent,
    PositionWaypoint,
    RotationWaypoint,
} from '@/stores/scenarioStore';
import { Section, NumberField } from './InspectorControls';
import DeleteIcon from '@mui/icons-material/Delete';
import EditIcon from '@mui/icons-material/Edit';
import { v4 as uuidv4 } from 'uuid';
import { PlatformComponentInspector } from './PlatformComponentInspector';

interface WaypointEditDialogProps {
    open: boolean;
    onClose: () => void;
    waypoint: PositionWaypoint | RotationWaypoint | null;
    waypointType: 'position' | 'rotation';
    onFieldChange: (field: string, value: number | null) => void;
}

function WaypointEditDialog({
    open,
    onClose,
    waypoint,
    waypointType,
    onFieldChange,
}: WaypointEditDialogProps) {
    if (!waypoint) return null;

    return (
        <Dialog open={open} onClose={onClose}>
            <DialogTitle>Edit Waypoint</DialogTitle>
            <DialogContent>
                <Box
                    sx={{
                        display: 'flex',
                        flexDirection: 'column',
                        gap: 2,
                        pt: 1,
                    }}
                >
                    {waypointType === 'position' && 'x' in waypoint && (
                        <>
                            <NumberField
                                label="X"
                                value={waypoint.x}
                                onChange={(v) => onFieldChange('x', v)}
                            />
                            <NumberField
                                label="Y"
                                value={waypoint.y}
                                onChange={(v) => onFieldChange('y', v)}
                            />
                            <NumberField
                                label="Altitude"
                                value={waypoint.altitude}
                                onChange={(v) => onFieldChange('altitude', v)}
                            />
                        </>
                    )}
                    {waypointType === 'rotation' && 'azimuth' in waypoint && (
                        <>
                            <NumberField
                                label="Azimuth (deg)"
                                value={waypoint.azimuth}
                                onChange={(v) => onFieldChange('azimuth', v)}
                            />
                            <NumberField
                                label="Elevation (deg)"
                                value={waypoint.elevation}
                                onChange={(v) => onFieldChange('elevation', v)}
                            />
                        </>
                    )}
                    <NumberField
                        label="Time (s)"
                        value={waypoint.time}
                        onChange={(v) => onFieldChange('time', v)}
                    />
                </Box>
            </DialogContent>
            <DialogActions>
                <Button onClick={onClose}>Close</Button>
            </DialogActions>
        </Dialog>
    );
}

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

    const [editingWaypointInfo, setEditingWaypointInfo] = useState<{
        type: 'position' | 'rotation';
        index: number;
    } | null>(null);

    const handleWaypointFieldChange = (field: string, value: number | null) => {
        if (!editingWaypointInfo) return;
        const { type, index } = editingWaypointInfo;
        const pathPrefix = type === 'position' ? 'motionPath' : 'rotation';
        handleChange(`${pathPrefix}.waypoints.${index}.${field}`, value);
    };

    const currentEditingWaypoint = editingWaypointInfo
        ? editingWaypointInfo.type === 'position'
            ? item.motionPath.waypoints[editingWaypointInfo.index]
            : item.rotation.type === 'path'
              ? item.rotation.waypoints[editingWaypointInfo.index]
              : null
        : null;

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
                                alignItems: 'center',
                                justifyContent: 'space-between',
                                p: 1,
                                border: 1,
                                borderColor: 'divider',
                                borderRadius: 1,
                            }}
                        >
                            <Typography
                                variant="body2"
                                sx={{
                                    flexGrow: 1,
                                    mr: 1,
                                    whiteSpace: 'nowrap',
                                    overflow: 'hidden',
                                    textOverflow: 'ellipsis',
                                }}
                            >
                                {`T: ${wp.time}s, X: ${wp.x}, Y: ${wp.y}, Alt: ${wp.altitude}`}
                            </Typography>
                            <Box sx={{ display: 'flex', flexShrink: 0 }}>
                                <IconButton
                                    size="small"
                                    onClick={() =>
                                        setEditingWaypointInfo({
                                            type: 'position',
                                            index: i,
                                        })
                                    }
                                >
                                    <EditIcon fontSize="small" />
                                </IconButton>
                                {allowMultiplePosWaypoints && (
                                    <IconButton
                                        size="small"
                                        onClick={() =>
                                            removePositionWaypoint(
                                                item.id,
                                                wp.id
                                            )
                                        }
                                        disabled={
                                            item.motionPath.waypoints.length <=
                                            1
                                        }
                                    >
                                        <DeleteIcon fontSize="small" />
                                    </IconButton>
                                )}
                            </Box>
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
                                                alignItems: 'center',
                                                justifyContent: 'space-between',
                                                p: 1,
                                                border: 1,
                                                borderColor: 'divider',
                                                borderRadius: 1,
                                            }}
                                        >
                                            <Typography
                                                variant="body2"
                                                sx={{
                                                    flexGrow: 1,
                                                    mr: 1,
                                                    whiteSpace: 'nowrap',
                                                    overflow: 'hidden',
                                                    textOverflow: 'ellipsis',
                                                }}
                                            >
                                                {`T: ${wp.time}s, Az: ${wp.azimuth}°, El: ${wp.elevation}°`}
                                            </Typography>
                                            <Box
                                                sx={{
                                                    display: 'flex',
                                                    flexShrink: 0,
                                                }}
                                            >
                                                <IconButton
                                                    size="small"
                                                    onClick={() =>
                                                        setEditingWaypointInfo({
                                                            type: 'rotation',
                                                            index: i,
                                                        })
                                                    }
                                                >
                                                    <EditIcon fontSize="small" />
                                                </IconButton>
                                                {allowMultipleWaypoints && (
                                                    <IconButton
                                                        size="small"
                                                        onClick={() =>
                                                            removeRotationWaypoint(
                                                                item.id,
                                                                wp.id
                                                            )
                                                        }
                                                        disabled={
                                                            rotation.waypoints
                                                                .length <= 1
                                                        }
                                                    >
                                                        <DeleteIcon fontSize="small" />
                                                    </IconButton>
                                                )}
                                            </Box>
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
                        value={
                            item.component.type === 'none'
                                ? ''
                                : item.component.type
                        }
                        onChange={(e) =>
                            setPlatformComponentType(
                                item.id,
                                e.target.value as PlatformComponent['type']
                            )
                        }
                    >
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
            <WaypointEditDialog
                open={!!editingWaypointInfo}
                onClose={() => setEditingWaypointInfo(null)}
                waypoint={currentEditingWaypoint}
                waypointType={editingWaypointInfo?.type ?? 'position'}
                onFieldChange={handleWaypointFieldChange}
            />
        </Box>
    );
}
