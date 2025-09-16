import {createTheme, responsiveFontSizes} from '@mui/material/styles';

// Create a theme instance.
let theme = createTheme({
    palette: {
        mode: 'dark', // Use 'light' or 'dark'
        primary: {
            main: '#90caf9',
        },
        secondary: {
            main: '#f48fb1',
        },
        background: {
            default: '#121212',
            paper: '#1e1e1e',
        },
    },
    typography: {
        fontFamily: ['Roboto', 'sans-serif'].join(','),
    },
});

theme = responsiveFontSizes(theme);

export default theme;
