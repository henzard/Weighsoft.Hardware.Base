# LED Example Project

## Overview

The LED Example Project is a **reference implementation** demonstrating Weighsoft's single-layer architecture pattern. It controls the built-in LED on ESP8266/ESP32 boards through four communication channels: REST, WebSocket, MQTT, and BLE (Phase 2).

**Purpose**: Use this project as a template for building your own Weighsoft services (scales, relays, sensors, displays).

## Architecture

### Single-Layer Pattern

The LED example uses Weighsoft's **single-layer architecture** - the application service (`LedExampleService`) directly composes framework components without separate settings services:

```
LedExampleService
├── HttpEndpoint<LedExampleState>      (REST API)
├── WebSocketTxRx<LedExampleState>     (Real-time updates)
├── MqttPubSub<LedExampleState>        (MQTT pub/sub)
└── BlePubSub<LedExampleState>         (BLE - Phase 2)
```

**Configuration is inline**:
- MQTT topics: Defined as String constants in the service
- BLE UUIDs: Defined inline (Phase 2)
- No separate `LedMqttSettingsService` or UI for config

### Multi-Channel Synchronization

All four channels share the same state (`LedExampleState`) and automatically synchronize using `StatefulService`'s origin tracking:

**Example Flow (WebSocket → MQTT)**:
1. User toggles LED via WebSocket
2. `WebSocketTxRx` calls `update(json, WS_ORIGIN_ID)`
3. State changes, `callUpdateHandlers(WS_ORIGIN_ID)` invoked
4. Update handler → `onConfigUpdated()` → Hardware LED updated
5. `MqttPubSub` checks `originId != MQTT_ORIGIN_ID` → Publishes to MQTT
6. `WebSocketTxRx` checks `originId == WS_ORIGIN_ID` → Skips broadcast (originator)
7. `BlePubSub` checks `originId != BLE_ORIGIN_ID` → Notifies BLE clients

**No feedback loops**: Origin tracking ensures originating channel doesn't re-broadcast.

## File Structure

### Backend (`src/examples/led/`)

#### `LedExampleService.h`
```cpp
class LedExampleService : public StatefulService<LedExampleState> {
 public:
  LedExampleService(AsyncWebServer* server, SecurityManager* securityManager, 
                    AsyncMqttClient* mqttClient);
  void begin();

 private:
  HttpEndpoint<LedExampleState> _httpEndpoint;
  MqttPubSub<LedExampleState> _mqttPubSub;
  WebSocketTxRx<LedExampleState> _webSocket;
  AsyncMqttClient* _mqttClient;

  // Inline MQTT configuration
  String _mqttBasePath;
  String _mqttName;
  String _mqttUniqueId;

  void configureMqtt();
  void onConfigUpdated();
};
```

**Key Points**:
- **State**: `LedExampleState` with `bool ledOn` field
- **Endpoints**: `/rest/ledExample` (REST), `/ws/ledExample` (WebSocket)
- **MQTT Topics**: Inline using `SettingValue::format("homeassistant/light/#{unique_id}")`
- **Origin Tracking**: Single `addUpdateHandler` for all channels

#### `LedExampleService.cpp`

**Constructor**:
- Initializes HTTP, WebSocket, MQTT components
- Sets inline MQTT topics: `_mqttBasePath`, `_mqttName`, `_mqttUniqueId`
- Registers MQTT connect callback: `_mqttClient->onConnect(configureMqtt)`
- Adds update handler: `addUpdateHandler([&](const String& originId) { onConfigUpdated(); })`

**`configureMqtt()`**:
- Builds MQTT topics from `_mqttBasePath`
- Calls `_mqttPubSub.configureTopics(pubTopic, subTopic)`
- Publishes Home Assistant auto-discovery JSON

**`onConfigUpdated()`**:
- Writes physical LED: `digitalWrite(LED_PIN, _state.ledOn ? LED_ON : LED_OFF)`

### Frontend (`interface/src/examples/led/`)

#### `types.ts`
```typescript
export interface LedExampleState {
  led_on: boolean;
}
```

#### `api.ts`
```typescript
export function readLedState(): AxiosPromise<LedExampleState> {
  return AXIOS.get('/ledExample');
}

export function updateLedState(ledState: LedExampleState): AxiosPromise<LedExampleState> {
  return AXIOS.post('/ledExample', ledState);
}
```

#### `LedExample.tsx`
Main router with tabs:
- Information
- REST Control
- WebSocket Control
- BLE Control (Phase 2)

#### `LedControlRest.tsx`
REST control form using `useRest` hook:
- Checkbox for LED state
- Save button to POST changes
- Manual refresh required

#### `LedControlWebSocket.tsx`
WebSocket control using `useWs` hook:
- Switch for LED state
- Real-time bidirectional updates
- Changes reflect instantly from any channel

#### `LedExampleInfo.tsx`
Information page explaining:
- Single-layer architecture
- Four communication channels
- Multi-channel sync and origin tracking
- File structure

## Testing Guide

### REST API

**Read LED State**:
```bash
curl http://192.168.3.118/rest/ledExample
```
Response:
```json
{"led_on":false}
```

**Update LED State**:
```bash
curl -X POST http://192.168.3.118/rest/ledExample \
  -H "Content-Type: application/json" \
  -d '{"led_on":true}'
```

### WebSocket

**Using websocat** (install: `cargo install websocat`):
```bash
websocat ws://192.168.3.118/ws/ledExample
```

**Receive updates** (JSON messages stream automatically):
```json
{"led_on":true}
```

**Send update**:
```json
{"led_on":false}
```

**Using Browser Console**:
```javascript
const ws = new WebSocket('ws://192.168.3.118/ws/ledExample');
ws.onmessage = (e) => console.log('LED State:', JSON.parse(e.data));
ws.send(JSON.stringify({led_on: true}));
```

### MQTT

**Subscribe** (using `mosquitto_sub`):
```bash
mosquitto_sub -h YOUR_MQTT_BROKER -t "homeassistant/light/led-12345678/state"
```

**Publish**:
```bash
mosquitto_pub -h YOUR_MQTT_BROKER \
  -t "homeassistant/light/led-12345678/set" \
  -m '{"state":"ON"}'
```

**Home Assistant Auto-Discovery**:
LED appears automatically in Home Assistant after MQTT connects. Check:
```bash
mosquitto_sub -h YOUR_MQTT_BROKER -t "homeassistant/light/led-12345678/config"
```

### BLE (Phase 2)

**Using nRF Connect (Mobile)**:
1. Scan for device: "Weighsoft-LED"
2. Connect
3. Find service: `LED_SERVICE_UUID`
4. Find characteristic: `LED_STATE_UUID`
5. Write JSON: `{"led_on":true}`
6. Enable notifications to receive updates

**Using `gatttool` (Linux)**:
```bash
gatttool -b AA:BB:CC:DD:EE:FF --char-write-req -a 0x0042 -n $(echo -n '{"led_on":true}' | xxd -p)
```

## Building Your Own Service

### Step 1: Copy the Template

```bash
cp -r src/examples/led src/examples/mydevice
cp -r interface/src/examples/led interface/src/examples/mydevice
```

### Step 2: Rename Classes

**Backend**:
- `LedExampleService` → `MyDeviceService`
- `LedExampleState` → `MyDeviceState`
- File names: `LedExampleService.*` → `MyDeviceService.*`

**Frontend**:
- `LedExample.tsx` → `MyDevice.tsx`
- `LedControlRest.tsx` → `MyDeviceControlRest.tsx`
- Update `types.ts`, `api.ts`

### Step 3: Modify State

**Backend** (`MyDeviceState` in `MyDeviceService.h`):
```cpp
class MyDeviceState {
 public:
  float weight;        // Example: scale reading
  bool tared;
  String unit;
  
  static void read(MyDeviceState& state, JsonObject& root) {
    root["weight"] = state.weight;
    root["tared"] = state.tared;
    root["unit"] = state.unit;
  }
  
  static StateUpdateResult update(JsonObject& root, MyDeviceState& state) {
    state.weight = root["weight"] | 0.0f;
    state.tared = root["tared"] | false;
    state.unit = root["unit"] | "kg";
    return StateUpdateResult::CHANGED;
  }
};
```

**Frontend** (`types.ts`):
```typescript
export interface MyDeviceState {
  weight: number;
  tared: boolean;
  unit: string;
}
```

### Step 4: Update Inline Configuration

**MQTT Topics**:
```cpp
_mqttBasePath = SettingValue::format("weighsoft/scale/#{unique_id}");
_mqttName = SettingValue::format("scale-#{unique_id}");
_mqttUniqueId = SettingValue::format("scale-#{unique_id}");
```

**BLE UUIDs** (Phase 2):
```cpp
_bleServiceUuid = "12345678-1234-5678-1234-56789abcdef0";
_bleCharUuid = "12345678-1234-5678-1234-56789abcdef1";
```

### Step 5: Implement Hardware Control

**`onConfigUpdated()`**:
```cpp
void MyDeviceService::onConfigUpdated() {
  if (_state.tared) {
    tareScale();
  }
  updateDisplay(_state.weight, _state.unit);
}
```

### Step 6: Update Endpoints

**Backend**:
```cpp
#define MYDEVICE_ENDPOINT_PATH "/rest/myDevice"
#define MYDEVICE_SOCKET_PATH "/ws/myDevice"
```

**Frontend API**:
```typescript
export function readDeviceState(): AxiosPromise<MyDeviceState> {
  return AXIOS.get('/myDevice');
}
```

### Step 7: Wire Up in `main.cpp`

```cpp
#include <examples/mydevice/MyDeviceService.h>

MyDeviceService* myDeviceService;

void setup() {
  // ... framework init ...
  
  myDeviceService = new MyDeviceService(
      server,
      esp8266React->getSecurityManager(),
      esp8266React->getMqttClient()
  );
  myDeviceService->begin();
}
```

### Step 8: Update Navigation

**`interface/src/project/ProjectMenu.tsx`**:
```typescript
<LayoutMenuItem icon={ScaleIcon} label="My Device" to={`/${PROJECT_PATH}/my-device`} />
```

**`interface/src/project/ProjectRouting.tsx`**:
```typescript
import MyDevice from '../examples/mydevice/MyDevice';

<Route path="my-device/*" element={<MyDevice />} />
```

## Best Practices

### 1. Keep Configuration Inline
Don't create separate settings services unless users truly need runtime configuration (rare).

### 2. Use Origin Tracking
Never manually check channel types. Let `StatefulService` handle it:

✅ **Do**:
```cpp
addUpdateHandler([&](const String& originId) { 
  onConfigUpdated();  // StatefulService handles origin tracking
}, false);
```

❌ **Don't**:
```cpp
addUpdateHandler([&](const String& originId) { 
  if (originId != "mqtt") mqttPublish();  // Manual checking
  if (originId != "ws") wsPublish();
}, false);
```

### 3. Validate in `update()`
Always validate input and return appropriate `StateUpdateResult`:

```cpp
static StateUpdateResult update(JsonObject& root, MyState& state) {
  float newValue = root["value"] | 0.0f;
  
  if (newValue < 0 || newValue > 1000) {
    return StateUpdateResult::ERROR;  // Invalid
  }
  
  if (state.value == newValue) {
    return StateUpdateResult::UNCHANGED;  // No change
  }
  
  state.value = newValue;
  return StateUpdateResult::CHANGED;
}
```

### 4. Use Const-Correctness
Match ESP32 compiler expectations:

```cpp
const AsyncWebHeader* header = request->getHeader("Authorization");
const AsyncWebParameter* param = request->getParam("value");
```

### 5. Test All Channels
Verify state sync across REST, WebSocket, MQTT, and BLE (Phase 2):
- Change via REST → Check WebSocket, MQTT, BLE receive update
- Change via MQTT → Check WebSocket, REST, BLE reflect change
- Change via BLE → Check REST, WebSocket, MQTT update

## Troubleshooting

### LED Not Changing

**Check Serial Output**:
```
[4/6] LED example service created OK
[4/6] LED example loaded OK
```

**Verify GPIO Pin**:
- NodeMCU: LED_PIN = 2 (GPIO2, inverted)
- ESP32-DevKit: LED_PIN = 2 (GPIO2, normal)

### MQTT Not Publishing

**Check MQTT Connection**:
- Verify `FT_MQTT 1` in `features.ini`
- Check MQTT broker settings in web UI
- Monitor serial: "Starting MQTT..."

**Verify Topics**:
```cpp
Serial.printf("MQTT Base: %s\n", _mqttBasePath.c_str());
```

### WebSocket Not Connecting

**Check Browser Console**:
```
WebSocket connection to 'ws://192.168.3.118/ws/ledExample' failed
```

**Verify**:
- Correct IP address
- Endpoint path matches backend: `/ws/ledExample`
- No firewall blocking WebSocket

### State Not Syncing

**Symptoms**: Changing LED via WebSocket doesn't update MQTT.

**Check Origin Tracking**:
- `addUpdateHandler` should NOT filter by `originId` manually
- Let `MqttPubSub` and `WebSocketTxRx` handle origin checks internally

## See Also

- [DESIGN-PATTERNS.md](DESIGN-PATTERNS.md) - Pattern 11: Single-Layer Protocol Integration
- [EXTENSION-GUIDE.md](EXTENSION-GUIDE.md) - Detailed service creation guide
- [API-REFERENCE.md](API-REFERENCE.md) - REST and WebSocket API documentation
- [CONFIGURATION.md](CONFIGURATION.md) - Feature flags and build configuration
