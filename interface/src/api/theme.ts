import axios from 'axios';

export interface UserTheme {
  theme: 'light' | 'dark';
}

export const ThemeApi = {
  getUserTheme: () => axios.get<UserTheme>('/rest/userTheme'),

  setUserTheme: (theme: 'light' | 'dark') =>
    axios.put<UserTheme>('/rest/userTheme', { theme })
};
