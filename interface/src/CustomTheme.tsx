import { FC, useMemo } from 'react';

import { CssBaseline } from '@mui/material';
import { createTheme, responsiveFontSizes, ThemeProvider } from '@mui/material/styles';
import { orange, red, green } from '@mui/material/colors';

import { RequiredChildrenProps } from './utils';
import { useThemeMode } from './contexts/theme';

// Weighsoft Brand Colors
const weighsoftColors = {
  curiousBlue: {
    main: '#2596D1',    // Primary brand color
    light: '#5BB4E3',   // Lighter variant
    dark: '#1976A8',    // Darker variant
    contrastText: '#FFFFFF'
  },
  bunting: {
    main: '#2B3449',    // Secondary brand color
    light: '#4E5A70',   // Lighter variant
    dark: '#1A2130',    // Darker variant
    contrastText: '#FFFFFF'
  }
};

const CustomTheme: FC<RequiredChildrenProps> = ({ children }) => {
  const { mode } = useThemeMode();

  const theme = useMemo(
    () => responsiveFontSizes(
      createTheme({
        palette: {
          mode,
          // Primary - Curious Blue (consistent across modes)
          primary: {
            main: weighsoftColors.curiousBlue.main,
            light: weighsoftColors.curiousBlue.light,
            dark: weighsoftColors.curiousBlue.dark,
            contrastText: weighsoftColors.curiousBlue.contrastText
          },
          // Secondary - Bunting (consistent across modes)
          secondary: {
            main: weighsoftColors.bunting.main,
            light: weighsoftColors.bunting.light,
            dark: weighsoftColors.bunting.dark,
            contrastText: weighsoftColors.bunting.contrastText
          },
          // Mode-specific backgrounds
          background: mode === 'light' ? {
            default: '#FAFAFA',
            paper: '#FFFFFF'
          } : {
            default: '#121212',
            paper: '#1E1E1E'
          },
          // Mode-specific text
          text: mode === 'light' ? {
            primary: 'rgba(0, 0, 0, 0.87)',
            secondary: 'rgba(0, 0, 0, 0.6)'
          } : {
            primary: '#FFFFFF',
            secondary: 'rgba(255, 255, 255, 0.7)'
          },
          // Status colors (mode-aware)
          info: {
            main: weighsoftColors.curiousBlue.main
          },
          warning: {
            main: mode === 'light' ? orange[700] : orange[300]
          },
          error: {
            main: mode === 'light' ? red[700] : red[300]
          },
          success: {
            main: mode === 'light' ? green[700] : green[400]
          },
          // Divider
          divider: mode === 'light' ? 'rgba(0, 0, 0, 0.12)' : 'rgba(255, 255, 255, 0.12)'
        },
        // Typography optimizations for both modes
        typography: {
          fontFamily: [
            '-apple-system',
            'BlinkMacSystemFont',
            '"Segoe UI"',
            'Roboto',
            '"Helvetica Neue"',
            'Arial',
            'sans-serif'
          ].join(','),
        },
        // Component overrides for better dark mode
        components: {
          MuiPaper: {
            styleOverrides: {
              root: {
                backgroundImage: 'none', // Remove default gradient in dark mode
              },
            },
          },
        },
      })
    ),
    [mode]
  );

  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      {children}
    </ThemeProvider>
  );
};

export default CustomTheme;
