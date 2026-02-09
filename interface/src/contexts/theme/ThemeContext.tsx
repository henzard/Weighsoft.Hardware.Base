import { createContext, FC, useContext, useState, useEffect } from 'react';
import { AuthenticationContext } from '../authentication';
import { ThemeApi } from '../../api/theme';

type ThemeMode = 'light' | 'dark';

interface ThemeContextType {
  mode: ThemeMode;
  toggleTheme: () => void;
  loading: boolean;
}

const ThemeContext = createContext<ThemeContextType | undefined>(undefined);

export const ThemeProvider: FC<{ children: React.ReactNode }> = ({ children }) => {
  const { me } = useContext(AuthenticationContext);
  const [mode, setMode] = useState<ThemeMode>(() => {
    // Initial: Check system preference
    if (window.matchMedia('(prefers-color-scheme: dark)').matches) {
      return 'dark';
    }
    return 'light';
  });
  const [loading, setLoading] = useState(false);

  // Load theme from ESP backend when user logs in
  useEffect(() => {
    if (me) {
      loadThemeFromBackend();
    }
  }, [me]);

  const loadThemeFromBackend = async () => {
    try {
      setLoading(true);
      const response = await ThemeApi.getUserTheme();
      setMode(response.data.theme);
    } catch (error) {
      console.error('Failed to load theme preference:', error);
      // Keep current/system preference on error
    } finally {
      setLoading(false);
    }
  };

  const toggleTheme = async () => {
    const newMode = mode === 'light' ? 'dark' : 'light';

    // Optimistic update for immediate UI feedback
    setMode(newMode);

    // Save to ESP backend if authenticated
    if (me) {
      try {
        await ThemeApi.setUserTheme(newMode);
      } catch (error) {
        console.error('Failed to save theme preference:', error);
        // Revert on error
        setMode(mode);
      }
    }
    // If not authenticated, just toggle locally (temporary)
  };

  return (
    <ThemeContext.Provider value={{ mode, toggleTheme, loading }}>
      {children}
    </ThemeContext.Provider>
  );
};

export const useThemeMode = () => {
  const context = useContext(ThemeContext);
  if (!context) {
    throw new Error('useThemeMode must be used within ThemeProvider');
  }
  return context;
};
