// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { createTheme, responsiveFontSizes } from '@mui/material/styles';

/**
 * FERS Central Color Palette.
 * These colors are used by both the Material UI DOM elements and the Three.js Canvas elements.
 * Importing this object directly allows usage without React Context hooks (useful for Three.js performance).
 */
export const fersColors = {
    background: {
        default: '#0d1117', // Deep blue-grey, better for technical apps than pure black
        paper: '#161b22', // Slightly lighter for panels
        canvas: '#202025', // Dark gray for the 3D void
        overlay: 'rgba(13, 17, 23, 0.85)', // Semi-transparent background for 3D HTML overlays/labels
    },
    primary: {
        main: '#29b6f6', // Light Blue - Main UI elements
        dark: '#01579b',
        light: '#4fc3f7',
    },
    secondary: {
        main: '#ff9100', // Amber - Highlights/Selection
        dark: '#c56000',
        light: '#ffc246',
    },
    text: {
        primary: '#e6edf3',
        secondary: '#8b949e',
    },
    // 3D Scene Specific Colors
    platform: {
        default: '#4fc3f7', // Matching primary light
        selected: '#ff9100', // Matching secondary main
        emission: '#ff9100',
    },
    physics: {
        velocity: '#00e676', // Bright Green
        boresight: '#ffea00', // Bright Yellow
        motionPath: '#ab47bc', // Purple
        rcs: '#ff9100', // Matches secondary.main for consistency
    },
    link: {
        monostatic: {
            strong: '#00e676', // Green
            weak: '#ef5350', // Red
        },
        illuminator: '#ffea00', // Yellow
        scattered: '#00bcd4', // Cyan
        direct: '#d500f9', // Violet
    },
};

// Create a theme instance.
let theme = createTheme({
    palette: {
        mode: 'dark', // Use 'light' or 'dark'
        primary: {
            main: fersColors.primary.main,
        },
        secondary: {
            main: fersColors.secondary.main,
        },
        background: {
            default: fersColors.background.default,
            paper: fersColors.background.paper,
        },
        text: {
            primary: fersColors.text.primary,
            secondary: fersColors.text.secondary,
        },
    },
    typography: {
        fontFamily: ['Roboto', 'sans-serif'].join(','),
    },
    components: {
        MuiCssBaseline: {
            styleOverrides: {
                body: {
                    scrollbarColor: '#30363d #0d1117',
                    '&::-webkit-scrollbar, & *::-webkit-scrollbar': {
                        backgroundColor: '#0d1117',
                        width: '8px',
                        height: '8px',
                    },
                    '&::-webkit-scrollbar-thumb, & *::-webkit-scrollbar-thumb':
                        {
                            borderRadius: 8,
                            backgroundColor: '#30363d',
                            minHeight: 24,
                        },
                    '&::-webkit-scrollbar-thumb:focus, & *::-webkit-scrollbar-thumb:focus':
                        {
                            backgroundColor: '#8b949e',
                        },
                },
            },
        },
    },
});

theme = responsiveFontSizes(theme);

export default theme;
