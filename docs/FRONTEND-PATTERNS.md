# Frontend Implementation Patterns

## Overview

This guide documents the correct import patterns, hook signatures, and component structures for frontend development in the Weighsoft Hardware UI. Use this as a reference to avoid common mistakes when implementing device projects.

## Import Patterns

### API Imports

#### AXIOS Instance

The centralized Axios instance handles JWT authentication automatically.

**✅ CORRECT:**

```typescript
// In API files (interface/src/api/mydevice.ts)
import { AXIOS } from './endpoints';

// In components (interface/src/examples/mydevice/*.tsx)
import { AXIOS } from '../../api/endpoints';
```

**❌ WRONG:**

```typescript
// File doesn't exist
import { AXIOS } from './axios-fetch';

// Don't import raw axios - bypasses JWT setup
import axios from 'axios';

// Don't use default import
import AXIOS from './endpoints';
```

**Why**: The `AXIOS` instance in `endpoints.ts` is pre-configured with:
- JWT token interceptor
- Base URL handling
- Error handling
- Request/response transformation

#### WebSocket Base URLs

**✅ CORRECT:**

```typescript
import { WEB_SOCKET_ROOT, WS_BASE_URL } from '../../api/endpoints';

// In component
const MY_WEBSOCKET_URL = `${WEB_SOCKET_ROOT}mydevice`;

// In endpoints.ts
export const MY_SOCKET_PATH = `${WS_BASE_URL}mydevice`;
```

**❌ WRONG:**

```typescript
// Hardcoding duplicates the /ws/ prefix
const MY_WEBSOCKET_URL = '/ws/mydevice';
export const MY_SOCKET_PATH = '/ws/mydevice';
```

**Why**: Using the base URL constant ensures consistency and makes it easy to change the WebSocket prefix globally if needed.

### Hook Imports

Hooks are located in the `utils/` directory, NOT `components/`.

**✅ CORRECT:**

```typescript
import { useRest, useWs } from '../../utils';
```

**❌ WRONG:**

```typescript
// Hooks are not in components
import { useRest, useWs } from '../../components';

// Don't import from individual files
import { useRest } from '../../utils/useRest';
```

**Available hooks**:
- `useRest<T>` - For configuration forms with manual save
- `useWs<T>` - For real-time monitoring via WebSocket
- `updateValue` - Helper for form field updates
- `extractErrorMessage` - Helper for error handling

### Component Imports

Shared UI components are in the `components/` directory.

**✅ CORRECT:**

```typescript
import { 
  SectionContent,      // Page section wrapper
  FormLoader,          // Loading spinner
  RouterTabs,          // Tab navigation
  useRouterTab,        // Tab routing hook
  useLayoutTitle,      // Page title hook
  BlockFormControlLabel, // Styled form control
  ButtonRow,           // Button layout helper
  MessageBox           // Error/success messages
} from '../../components';
```

**❌ WRONG:**

```typescript
// Don't import from subdirectories
import { SectionContent } from '../../components/layout/SectionContent';

// Don't create duplicate components
import { MyCustomLoader } from './MyCustomLoader';  // Use FormLoader instead
```

**Why**: Shared components ensure consistent UI/UX across the application.

## Hook Usage Patterns

### useRest Hook

For configuration forms where users manually save changes.

**Signature:**

```typescript
const { 
  loadData: () => Promise<void>,
  saveData: (data: T) => Promise<void>,
  data: T | undefined,
  setData: (data: T) => void
} = useRest<T>({ 
  read: () => AxiosPromise<T> 
});
```

**Complete Example:**

```typescript
import { useRest } from '../../utils';
import { readMyDeviceSettings, updateMyDeviceSettings } from '../../api/mydevice';
import { MyDeviceSettings } from '../../types/mydevice';

const MyDeviceSettingsForm: FC = () => {
  const { loadData, saveData, data, setData } = useRest<MyDeviceSettings>({
    read: readMyDeviceSettings
  });

  // Loading state
  if (!data) {
    return <FormLoader />;
  }

  // Handle form submission
  const handleSubmit = async () => {
    await saveData(data);
  };

  // Handle field change
  const handleChange = (field: keyof MyDeviceSettings, value: any) => {
    setData({ ...data, [field]: value });
  };

  return (
    <SectionContent title="Settings">
      <TextField 
        value={data.name} 
        onChange={(e) => handleChange('name', e.target.value)}
      />
      <Button onClick={handleSubmit}>Save</Button>
    </SectionContent>
  );
};
```

**Common Mistakes:**

```typescript
// ❌ WRONG - 'loading' doesn't exist
const { data, loading } = useRest<T>({ read });

// ❌ WRONG - 'error' doesn't exist
const { data, error } = useRest<T>({ read });

// ✅ CORRECT - Check data for loading state
const { data } = useRest<T>({ read });
if (!data) return <FormLoader />;
```

**When to use**: Settings pages, configuration forms, anything with a "Save" button.

### useWs Hook

For real-time status monitoring via WebSocket.

**Signature:**

```typescript
const { 
  connected: boolean,
  updateData: (data: Partial<T>) => void,
  data: T | undefined
} = useWs<T>(websocketUrl: string);
```

**Complete Example:**

```typescript
import { useWs } from '../../utils';
import { WEB_SOCKET_ROOT } from '../../api/endpoints';
import { MyDeviceData } from '../../types/mydevice';

const MY_WEBSOCKET_URL = `${WEB_SOCKET_ROOT}mydevice`;

const MyDeviceMonitor: FC = () => {
  const { connected, updateData, data } = useWs<MyDeviceData>(MY_WEBSOCKET_URL);

  if (!data) {
    return <FormLoader />;
  }

  // For bidirectional WebSocket, send updates
  const handleToggle = () => {
    updateData({ enabled: !data.enabled });
  };

  return (
    <SectionContent title="Live Monitor">
      {!connected && <Alert severity="warning">Disconnected</Alert>}
      <Typography>Temperature: {data.temperature}°C</Typography>
      <Switch checked={data.enabled} onChange={handleToggle} />
    </SectionContent>
  );
};
```

**Common Mistakes:**

```typescript
// ❌ WRONG - Pass string URL, not object
const { data } = useWs<T>({ url: WEBSOCKET_URL });

// ❌ WRONG - Don't hardcode WebSocket path
const { data } = useWs<T>('/ws/mydevice');

// ✅ CORRECT - Pass URL string directly
const WEBSOCKET_URL = `${WEB_SOCKET_ROOT}mydevice`;
const { data } = useWs<T>(WEBSOCKET_URL);
```

**When to use**: Live dashboards, real-time monitoring, status displays, anything that auto-updates.

### Decision Tree: useRest vs useWs

```
Does the user need to click "Save"?
├─ YES → useRest (configuration forms)
└─ NO  → useWs (live monitoring)

Is the data updated frequently (< 5s)?
├─ YES → useWs (real-time)
└─ NO  → useRest (polling)

Does the data change only when user acts?
├─ YES → useRest (manual control)
└─ NO  → useWs (automatic updates)
```

## WebSocket URL Construction

### In Component Files

Define the WebSocket URL using the `WEB_SOCKET_ROOT` constant:

```typescript
import { WEB_SOCKET_ROOT } from '../../api/endpoints';

const MY_WEBSOCKET_URL = `${WEB_SOCKET_ROOT}mydevice`;
const { connected, data } = useWs<MyData>(MY_WEBSOCKET_URL);
```

### In endpoints.ts

Export WebSocket paths using `WS_BASE_URL`:

```typescript
// interface/src/api/endpoints.ts
export const WS_BASE_URL = '/ws/';
export const WEB_SOCKET_ROOT = `${window.location.protocol === 'https:' ? 'wss:' : 'ws:'}//${window.location.host}${WS_BASE_URL}`;

// Device-specific paths
export const LED_SOCKET_PATH = `${WS_BASE_URL}ledExample`;
export const SERIAL_SOCKET_PATH = `${WS_BASE_URL}serial`;
export const MY_DEVICE_SOCKET_PATH = `${WS_BASE_URL}mydevice`;
```

**Why**: This pattern ensures:
- Correct protocol (ws:// or wss://)
- Correct host and port
- Consistent WebSocket prefix
- Easy to change globally

## Component Structure Patterns

### Standard Device Project Structure

Every device project should follow this 5-file pattern:

```
interface/src/examples/mydevice/
├── MyDevice.tsx          # Main router with tabs
├── MyDeviceInfo.tsx      # Documentation/info tab
├── MyDeviceRest.tsx      # REST polling tab (optional)
├── MyDeviceWebSocket.tsx # Real-time monitoring tab
└── MyDeviceBle.tsx       # BLE connection instructions
```

Plus API layer:

```
interface/src/api/mydevice.ts    # API functions
interface/src/types/mydevice.ts  # TypeScript interfaces
```

### Main Router Pattern (MyDevice.tsx)

**Template:**

```typescript
import { FC } from 'react';
import { Navigate, Route, Routes } from 'react-router-dom';
import { Tab } from '@mui/material';
import { RouterTabs, useRouterTab, useLayoutTitle } from '../../components';

import MyDeviceInfo from './MyDeviceInfo';
import MyDeviceWebSocket from './MyDeviceWebSocket';
import MyDeviceBle from './MyDeviceBle';

const MyDevice: FC = () => {
  useLayoutTitle('My Device');  // Concise label (not "My Device Example")
  const { routerTab } = useRouterTab();

  return (
    <>
      <RouterTabs value={routerTab}>
        <Tab value="" label="Info" />
        <Tab value="websocket" label="Monitor" />
        <Tab value="ble" label="BLE" />
      </RouterTabs>
      <Routes>
        <Route path="" element={<MyDeviceInfo />} />
        <Route path="websocket" element={<MyDeviceWebSocket />} />
        <Route path="ble" element={<MyDeviceBle />} />
        <Route path="*" element={<Navigate replace to="" />} />
      </Routes>
    </>
  );
};

export default MyDevice;
```

### Info Tab Pattern (MyDeviceInfo.tsx)

**Template:**

```typescript
import React, { FC } from 'react';
import { Typography, List, ListItem, ListItemText, Alert } from '@mui/material';
import { SectionContent } from '../../components';

const MyDeviceInfo: FC = () => (
  <SectionContent title="My Device">
    <Typography variant="body1" paragraph>
      Brief description of what this device does.
    </Typography>

    <Typography variant="h6">Features</Typography>
    <List>
      <ListItem>
        <ListItemText 
          primary="Feature 1" 
          secondary="Description of feature 1" 
        />
      </ListItem>
      {/* More features */}
    </List>

    <Alert severity="info">
      Hardware setup instructions or important notes.
    </Alert>
  </SectionContent>
);

export default MyDeviceInfo;
```

### Real-Time Monitor Pattern (MyDeviceWebSocket.tsx)

**Template:**

```typescript
import React, { FC } from 'react';
import { Typography, Box, Alert } from '@mui/material';
import { SectionContent, FormLoader } from '../../components';
import { WEB_SOCKET_ROOT } from '../../api/endpoints';
import { useWs } from '../../utils';
import { MyDeviceData } from '../../types/mydevice';

const WEBSOCKET_URL = `${WEB_SOCKET_ROOT}mydevice`;

const MyDeviceWebSocket: FC = () => {
  const { connected, data } = useWs<MyDeviceData>(WEBSOCKET_URL);

  if (!data) {
    return <FormLoader message="Connecting to device..." />;
  }

  return (
    <SectionContent title="Live Monitor">
      {!connected && (
        <Alert severity="warning">
          WebSocket disconnected. Reconnecting...
        </Alert>
      )}
      
      <Box>
        <Typography variant="h6">Current Status</Typography>
        <Typography>Value: {data.value}</Typography>
        <Typography>Timestamp: {new Date(data.timestamp).toLocaleString()}</Typography>
      </Box>
    </SectionContent>
  );
};

export default MyDeviceWebSocket;
```

### BLE Instructions Pattern (MyDeviceBle.tsx)

**Template:**

```typescript
import React, { FC } from 'react';
import { Typography, List, ListItem, ListItemText, Alert } from '@mui/material';
import { SectionContent } from '../../components';

const MyDeviceBle: FC = () => (
  <SectionContent title="BLE Connection">
    <Typography variant="body1" paragraph>
      Connect to this device via Bluetooth Low Energy using the following UUIDs:
    </Typography>

    <Typography variant="h6">Service UUID</Typography>
    <Typography fontFamily="monospace">
      12340000-e8f2-537e-4f6c-d104768a1234
    </Typography>

    <Typography variant="h6" sx={{ mt: 2 }}>Characteristic UUID</Typography>
    <Typography fontFamily="monospace">
      12340001-e8f2-537e-4f6c-d104768a1234
    </Typography>

    <Alert severity="info" sx={{ mt: 2 }}>
      Use a BLE scanning app like nRF Connect (iOS/Android) to discover and connect.
    </Alert>

    <Typography variant="h6" sx={{ mt: 2 }}>Supported Operations</Typography>
    <List>
      <ListItem>
        <ListItemText 
          primary="Read" 
          secondary="Read current device state" 
        />
      </ListItem>
      <ListItem>
        <ListItemText 
          primary="Notify" 
          secondary="Receive automatic updates when state changes" 
        />
      </ListItem>
    </List>
  </SectionContent>
);

export default MyDeviceBle;
```

## Type Definition Patterns

### Backend to Frontend Type Mapping

Backend state (C++) uses **snake_case** in JSON, frontend (TypeScript) uses **camelCase**. Axios automatically converts between them.

**Backend State:**

```cpp
class MyDeviceState {
public:
  float temperature;
  bool heater_enabled;
  unsigned long last_update;
  
  static void read(MyDeviceState& state, JsonObject& root) {
    root["temperature"] = state.temperature;        // snake_case
    root["heater_enabled"] = state.heaterEnabled;   // snake_case
    root["last_update"] = state.lastUpdate;         // snake_case
  }
};
```

**Frontend Type:**

```typescript
// interface/src/types/mydevice.ts
export interface MyDeviceData {
  temperature: number;      // Matches temperature
  heaterEnabled: boolean;   // Matches heater_enabled (auto-converts)
  lastUpdate: number;       // Matches last_update (auto-converts)
}
```

**Axios automatically handles the conversion!**

## API Layer Patterns

### Standard API File Structure

```typescript
// interface/src/api/mydevice.ts
import { AxiosPromise } from 'axios';
import { AXIOS } from './endpoints';
import { MyDeviceData, MyDeviceSettings } from '../types/mydevice';

// Endpoint constants
export const MY_DEVICE_ENDPOINT = '/rest/myDevice';

// Read functions
export function readMyDeviceData(): AxiosPromise<MyDeviceData> {
  return AXIOS.get(MY_DEVICE_ENDPOINT);
}

// Write functions (for bidirectional devices)
export function updateMyDeviceData(data: MyDeviceData): AxiosPromise<MyDeviceData> {
  return AXIOS.post(MY_DEVICE_ENDPOINT, data);
}

// Settings functions (if needed)
export function readMyDeviceSettings(): AxiosPromise<MyDeviceSettings> {
  return AXIOS.get(`${MY_DEVICE_ENDPOINT}/settings`);
}

export function updateMyDeviceSettings(settings: MyDeviceSettings): AxiosPromise<MyDeviceSettings> {
  return AXIOS.put(`${MY_DEVICE_ENDPOINT}/settings`, settings);
}
```

## Menu and Routing Integration

### Adding to Menu (ProjectMenu.tsx)

```typescript
import MyDeviceIcon from '@mui/icons-material/DeviceThermostat';

<LayoutMenuItem 
  path="/my-device" 
  label="My Device"        // Concise label
  icon={MyDeviceIcon} 
/>
```

**Label Guidelines**:
- ✅ "LED" (not "LED Example")
- ✅ "Serial" (not "Serial Monitor")
- ✅ "My Device" (not "My Device Control Panel")

### Adding to Routing (ProjectRouting.tsx)

```typescript
import MyDevice from '../examples/mydevice/MyDevice';

<Route path="my-device/*" element={<MyDevice />} />
```

**Path Guidelines**:
- Use kebab-case for URLs: `my-device`, `led-control`
- Match the menu path exactly
- Include `/*` wildcard for nested routes

## Common Patterns Summary

### Imports Checklist

```typescript
// ✅ API imports
import { AXIOS } from '../../api/endpoints';

// ✅ Hook imports
import { useRest, useWs } from '../../utils';

// ✅ Component imports
import { SectionContent, FormLoader } from '../../components';

// ✅ WebSocket URL
import { WEB_SOCKET_ROOT } from '../../api/endpoints';

// ✅ Type imports
import { MyDeviceData } from '../../types/mydevice';
```

### Hook Usage Checklist

```typescript
// ✅ useRest for manual save
const { data, setData, saveData } = useRest<T>({ read });
if (!data) return <FormLoader />;

// ✅ useWs for real-time
const WEBSOCKET_URL = `${WEB_SOCKET_ROOT}mydevice`;
const { connected, data } = useWs<T>(WEBSOCKET_URL);
if (!data) return <FormLoader />;
```

### File Structure Checklist

```
✅ MyDevice.tsx           (main router)
✅ MyDeviceInfo.tsx       (documentation)
✅ MyDeviceWebSocket.tsx  (real-time)
✅ MyDeviceBle.tsx        (BLE info)
✅ api/mydevice.ts        (API layer)
✅ types/mydevice.ts      (TypeScript types)
```

## Troubleshooting

### "Cannot find module './axios-fetch'"

**Problem**: Wrong import path for AXIOS.

**Solution**:
```typescript
// ✅ CORRECT
import { AXIOS } from './endpoints';
```

### "Cannot find module '../../components'" for hooks

**Problem**: Hooks are in `utils/`, not `components/`.

**Solution**:
```typescript
// ✅ CORRECT
import { useRest, useWs } from '../../utils';
```

### "Property 'loading' does not exist on type..."

**Problem**: `useRest` doesn't return a `loading` property.

**Solution**:
```typescript
// ✅ CORRECT
const { data } = useRest<T>({ read });
if (!data) return <FormLoader />;
```

### WebSocket not connecting

**Problem**: Hardcoded WebSocket path or wrong URL construction.

**Solution**:
```typescript
// ✅ CORRECT
import { WEB_SOCKET_ROOT } from '../../api/endpoints';
const WEBSOCKET_URL = `${WEB_SOCKET_ROOT}mydevice`;
const { connected, data } = useWs<T>(WEBSOCKET_URL);
```

## Reference Files

- **LED Example**: `interface/src/examples/led/` - Bidirectional control pattern
- **Serial Example**: `interface/src/examples/serial/` - Read-only streaming pattern
- **Extension Guide**: `docs/EXTENSION-GUIDE.md` - Complete implementation guide
- **Device Template Guide**: `docs/DEVICE-TEMPLATE-GUIDE.md` - Quick start checklist
