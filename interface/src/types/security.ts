export interface User {
  username: string;
  password: string;
  admin: boolean;
  theme_preference: 'light' | 'dark';
}

export interface SecuritySettings {
  users: User[];
  jwt_secret: string;
}
