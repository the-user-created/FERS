// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import {
    Box,
    Checkbox,
    FormControl,
    FormControlLabel,
    FormGroup,
    TextField,
    Typography,
} from '@mui/material';
import { useScenarioStore, GlobalParameters } from '@/stores/scenarioStore';
import { NumberField } from './InspectorControls';

export function GlobalParametersInspector({
    item,
}: {
    item: GlobalParameters;
}) {
    const { updateItem } = useScenarioStore.getState();
    const handleChange = (path: string, value: unknown) =>
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
                label="Output Sampling Rate (Hz)"
                value={item.rate}
                onChange={(v) => handleChange('rate', v)}
            />
            <NumberField
                label="Internal Sim Sampling Rate (Hz)"
                value={item.simSamplingRate}
                onChange={(v) => handleChange('simSamplingRate', v)}
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
