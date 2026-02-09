# Design Patterns Catalog

## Overview

This document catalogs the key design patterns used in the ESP8266-React framework with implementation examples. Follow these patterns when extending the framework to ensure consistency and maintainability.

## Pattern 1: StatefulService Template Pattern

**Intent**: Provide a reusable base class for managing state with thread-safe access and event-driven updates.

**When to Use**:
- Managing any configuration or runtime state
- Need automatic synchronization across multiple interfaces
- Require thread-safe state access (ESP32)

**Structure**:
```cpp
template <class T>
class StatefulService {
    T _state;
    std::list<StateUpdateHandlerInfo_t> _updateHandlers;
    // Methods for update, read, handler management
};
```

**Implementation Example**:
```cpp
// 1. Define state class
class TemperatureSettings {
public:
    float threshold;
    bool alertEnabled;
    
    static void read(TemperatureSettings& settings, JsonObject& root) {
        root["threshold"] = settings.threshold;
        root["alert_enabled"] = settings.alertEnabled;
    }
    
    static StateUpdateResult update(JsonObject& root, TemperatureSettings& settings) {
        settings.threshold = root["threshold"] | 25.0;
        settings.alertEnabled = root["alert_enabled"] | false;
        return StateUpdateResult::CHANGED;
    }
};

// 2. Create service inheriting StatefulService
class TemperatureService : public StatefulService<TemperatureSettings> {
public:
    TemperatureService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager);
    void begin();
    
private:
    HttpEndpoint<TemperatureSettings> _httpEndpoint;
    FSPersistence<TemperatureSettings> _fsPersistence;
};
```

**Benefits**:
- Consistent state management
- Automatic propagation
- Thread-safe on ESP32
- Reusable infrastructure

## Pattern 2: Service Composition Pattern

**Intent**: Build services by composing infrastructure components rather than inheritance.

**When to Use**:
- Creating new services
- Need REST API, persistence, WebSocket, or MQTT
- Want flexible feature combinations

**Structure**:
```cpp
class MyService : public StatefulService<MySettings> {
    HttpEndpoint<MySettings> _httpEndpoint;      // REST API
    FSPersistence<MySettings> _fsPersistence;    // File persistence
    WebSocketTxRx<MySettings> _webSocket;        // WebSocket
    MqttPubSub<MySettings> _mqttPubSub;          // MQTT
};
```

**Implementation Example**:
```cpp
TemperatureService::TemperatureService(
    AsyncWebServer* server, 
    FS* fs, 
    SecurityManager* securityManager
) :
    _httpEndpoint(
        TemperatureSettings::read,
        TemperatureSettings::update,
        this,
        server,
        "/rest/temperatureSettings",
        securityManager,
        AuthenticationPredicates::IS_ADMIN
    ),
    _fsPersistence(
        TemperatureSettings::read,
        TemperatureSettings::update,
        this,
        fs,
        "/config/temperatureSettings.json"
    )
{
    // Additional initialization
    addUpdateHandler([&](const String& originId) {
        // React to temperature settings changes
        configureAlerts();
    });
}
```

**Benefits**:
- Pick only needed features
- Clear separation of concerns
- Easy to test individual components
- Follows composition over inheritance

## Pattern 3: JSON Serialization Functions Pattern

**Intent**: Provide static functions for bidirectional JSON conversion.

**When to Use**:
- Every state class
- Need REST API or MQTT integration
- Filesystem persistence

**Structure**:
```cpp
class Settings {
public:
    // State fields
    
    static void read(Settings& settings, JsonObject& root);
    static StateUpdateResult update(JsonObject& root, Settings& settings);
};
```

**Implementation Example**:
```cpp
class TemperatureSettings {
public:
    float threshold;
    bool alertEnabled;
    String email;
    
    static void read(TemperatureSettings& settings, JsonObject& root) {
        root["threshold"] = settings.threshold;
        root["alert_enabled"] = settings.alertEnabled;
        root["email"] = settings.email;
    }
    
    static StateUpdateResult update(JsonObject& root, TemperatureSettings& settings) {
        // Parse with defaults
        settings.threshold = root["threshold"] | 25.0;
        settings.alertEnabled = root["alert_enabled"] | false;
        settings.email = root["email"] | "";
        
        // Validation
        if (settings.threshold < -50 || settings.threshold > 150) {
            return StateUpdateResult::ERROR;
        }
        
        return StateUpdateResult::CHANGED;
    }
};
```

**Benefits**:
- Centralized serialization logic
- Type-safe
- Reusable across infrastructure
- Easy validation

## Pattern 4: Update Handler Registration Pattern

**Intent**: React to state changes through registered callbacks.

**When to Use**:
- Hardware control based on config
- Cascading updates
- Logging or notifications

**Structure**:
```cpp
statefulService->addUpdateHandler([&](const String& originId) {
    // React to state change
}, allowRemove);
```

**Implementation Example**:
```cpp
class TemperatureService : public StatefulService<TemperatureSettings> {
public:
    TemperatureService() {
        // Register handler for temperature changes
        addUpdateHandler([&](const String& originId) {
            read([&](TemperatureSettings& settings) {
                if (currentTemp > settings.threshold && settings.alertEnabled) {
                    sendAlert();
                }
            });
        });
    }
    
private:
    void sendAlert() {
        // Send email or notification
    }
};
```

**Best Practices**:
- Use `allowRemove=false` for infrastructure handlers (FSPersistence, WebSocketTx)
- Use `allowRemove=true` (default) for application logic
- Check origin ID to prevent loops
- Keep handlers fast (avoid blocking operations)

## Pattern 5: Origin Tracking Pattern

**Intent**: Prevent circular updates when state changes propagate across multiple channels.

**When to Use**:
- Publishing to MQTT
- Broadcasting to WebSockets
- Any bidirectional communication

**Structure**:
```cpp
void onUpdate(const String& originId) {
    if (originId != "mqtt") {
        publishToMqtt();
    }
}
```

**Implementation Example**:
```cpp
class LightStateService : public StatefulService<LightState> {
    MqttPubSub<LightState> _mqttPubSub;
    
    LightStateService() {
        addUpdateHandler([&](const String& originId) {
            // Only update hardware, not MQTT
            // (MqttPubSub handles publishing automatically)
            read([&](LightState& state) {
                digitalWrite(LED_PIN, state.ledOn ? HIGH : LOW);
            });
        });
    }
};
```

**Origin ID Values**:
- `"http"` - REST API
- `"mqtt"` - MQTT message
- `"websocket:{id}"` - WebSocket client
- `"internal"` - Code initialization

**Benefits**:
- Prevents infinite loops
- Enables bidirectional sync
- Clear update source tracking

## Pattern 6: Factory Settings with Placeholders Pattern

**Intent**: Provide sensible defaults with dynamic value substitution.

**When to Use**:
- Need unique values per device
- Default configurations
- Factory reset behavior

**Structure**:
```cpp
settings.clientId = root["client_id"] | SettingValue::format("#{platform}-#{unique_id}");
```

**Available Placeholders**:
- `#{platform}` - "esp8266" or "esp32"
- `#{unique_id}` - MAC-derived unique ID
- `#{random}` - Random hex string

**Implementation Example**:
```cpp
static StateUpdateResult update(JsonObject& root, MqttSettings& settings) {
    settings.clientId = root["client_id"] | 
        SettingValue::format("#{platform}-#{unique_id}");
    settings.hostname = root["hostname"] | 
        SettingValue::format("esp-#{unique_id}");
    
    return StateUpdateResult::CHANGED;
}
```

**Benefits**:
- Unique defaults per device
- No hardcoded values
- Factory reset friendly

## Pattern 7: React useRest Hook Pattern

**Intent**: Manage server state in React components with loading states.

**When to Use**:
- Configuration forms
- Need to read and update server state
- Manual save button

**Structure**:
```typescript
const { data, setData, saveData, saving, loadData } = 
    useRest<DataType>(endpoint, initialData);
```

**Implementation Example**:
```typescript
const TemperatureSettings: FC = () => {
    const { data, setData, saveData, saving } = 
        useRest<TemperatureSettings>(TEMPERATURE_ENDPOINT);
    
    const updateValue = (name: string, value: any) => {
        setData({ ...data, [name]: value });
    };
    
    return (
        <form>
            <TextField
                value={data?.threshold || 25}
                onChange={(e) => updateValue('threshold', parseFloat(e.target.value))}
            />
            <Button onClick={saveData} disabled={saving}>
                Save
            </Button>
        </form>
    );
};
```

**Benefits**:
- Automatic loading on mount
- Loading state management
- Error handling with snackbar

## Pattern 8: React useWs Hook Pattern

**Intent**: Real-time bidirectional state sync via WebSocket.

**When to Use**:
- Real-time status display
- Live control (switches, sliders)
- Multiple clients need sync

**Structure**:
```typescript
const { data, updateData, connected } = 
    useWs<DataType>(endpoint, initialData);
```

**Implementation Example**:
```typescript
const LightControl: FC = () => {
    const { data, updateData, connected } = 
        useWs<LightState>("/ws/lightState", { led_on: false });
    
    const handleToggle = (event: React.ChangeEvent<HTMLInputElement>) => {
        updateData({ led_on: event.target.checked });
    };
    
    return (
        <Switch
            checked={data?.led_on || false}
            onChange={handleToggle}
            disabled={!connected}
        />
    );
};
```

**Benefits**:
- Real-time sync
- Optimistic updates
- Automatic reconnection
- Client ID tracking

## Pattern 9: Authentication Context Pattern

**Intent**: Manage global authentication state in React.

**When to Use**:
- Need user info across app
- JWT token management
- Protected routes

**Structure**:
```typescript
const { me, signIn, signOut } = useContext(AuthenticationContext);
```

**Implementation Example**:
```typescript
const SignInPage: FC = () => {
    const { signIn } = useContext(AuthenticationContext);
    const [username, setUsername] = useState('');
    const [password, setPassword] = useState('');
    
    const handleSubmit = async () => {
        try {
            await signIn(username, password, true);
            navigate('/dashboard');
        } catch (error) {
            // Show error
        }
    };
    
    return (
        <form onSubmit={handleSubmit}>
            {/* form fields */}
        </form>
    );
};
```

**Benefits**:
- Centralized auth state
- Automatic token injection
- Easy access control

## Pattern 10: Require Authentication Route Wrapper Pattern

**Intent**: Protect routes based on authentication status.

**When to Use**:
- Protected pages
- Admin-only sections
- Role-based access

**Structure**:
```typescript
<Route element={<RequireAdmin><Component /></RequireAdmin>} />
```

**Implementation Example**:
```typescript
<Routes>
    <Route 
        path="/security/*" 
        element={
            <RequireAdmin>
                <SecurityManagement />
            </RequireAdmin>
        } 
    />
    <Route 
        path="/dashboard" 
        element={
            <RequireAuthenticated>
                <Dashboard />
            </RequireAuthenticated>
        } 
    />
</Routes>
```

**Benefits**:
- Declarative access control
- Automatic redirects
- Clean route definitions

## Anti-Patterns to Avoid

### 1. Direct State Access

**Don't**:
```cpp
digitalWrite(LED_PIN, service._state.ledOn);  // Not thread-safe!
```

**Do**:
```cpp
service.read([&](LightState& state) {
    digitalWrite(LED_PIN, state.ledOn ? HIGH : LOW);
});
```

### 2. Ignoring StateUpdateResult

**Don't**:
```cpp
static StateUpdateResult update(JsonObject& root, Settings& settings) {
    settings.value = root["value"];
    return StateUpdateResult::CHANGED;  // Always CHANGED!
}
```

**Do**:
```cpp
static StateUpdateResult update(JsonObject& root, Settings& settings) {
    int newValue = root["value"] | 0;
    if (newValue < 0 || newValue > 100) {
        return StateUpdateResult::ERROR;  // Validation
    }
    if (settings.value == newValue) {
        return StateUpdateResult::UNCHANGED;  // No change
    }
    settings.value = newValue;
    return StateUpdateResult::CHANGED;
}
```

### 3. Creating Update Loops

**Don't**:
```cpp
addUpdateHandler([&](const String& originId) {
    // Always publish, creates loop!
    mqttClient->publish(topic, payload);
});
```

**Do**:
```cpp
addUpdateHandler([&](const String& originId) {
    if (originId != "mqtt") {
        mqttClient->publish(topic, payload);
    }
});
```

## Next Steps

- [EXTENSION-GUIDE.md](EXTENSION-GUIDE.md) - Apply these patterns
- [C4-CODE-PATTERNS.md](C4-CODE-PATTERNS.md) - See diagrams
- [API-REFERENCE.md](API-REFERENCE.md) - API contracts
