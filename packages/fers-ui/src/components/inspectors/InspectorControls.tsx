// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import React from 'react';
import {
    Accordion,
    AccordionSummary,
    AccordionDetails,
    Typography,
    TextField,
    Box,
} from '@mui/material';
import ExpandMoreIcon from '@mui/icons-material/ExpandMore';
import { open } from '@tauri-apps/plugin-dialog';

export const Section = ({
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

export const NumberField = ({
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

export const FileInput = ({
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
