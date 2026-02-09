# LCD Display Project

## Overview

The LCD Display Project controls a **YwRobot LCM1602** 16x2 I2C LCD display through four communication channels: REST, WebSocket, MQTT, and BLE. It follows the same **single-layer architecture** as the LED example but adds bidirectional text control with two display lines, configurable I2C address, and backlight control.

**Purpose**: Demonstrate bidirectional device control with multi-field state, I2C hardware integration, and real-time text updates across all channels.

## Features

- **Two-line text display**: Control each line independently (16 characters max per line)
- **I2C address configurable**: Switch between 0x27 and 0x3F from the UI
- **Backlight control**: Toggle backlight on/off via any channel
- **Real-time updates**: WebSocket mode pushes text changes instantly
- **Multi-channel sync**: REST, WebSocket, MQTT, and BLE all share the same state
- **Visual preview**: UI shows a live preview of the LCD content

## Hardware Setup

### Components

| Component | Description |
|-----------|-------------|
| YwRobot LCM1602 | 16x2 LCD with I2C backpack (PCF8574) |
| ESP32 board | e.g. Node32s, ESP32-DevKit |
| Jumper wires | 4x female-to-female |

### Wiring

| LCD Pin | ESP32 Pin | Description |
|---------|-----------|-------------|
| SDA | GPIO21 | I2C Data |
| SCL | GPIO22 | I2C Clock |
| VCC | 5V | Power |
| GND | GND | Ground |

> **Note**: Some ESP32 boards label SDA/SCL differently. Check your board's pinout diagram.

### I2C Address Detection

Most YwRobot LCM1602 modules use address **0x27**. Some variants use **0x3F**. You can detect the address using an I2C scanner sketch or by trying both addresses in the UI.

**Common addresses by manufacturer**:
- **0x27**: YwRobot, SunFounder, most generic modules
- **0x3F**: Some HiLetgo and Elegoo variants

If the display doesn't show text after uploading, try switching the I2C address in the Display Control page.

## Architecture

### Single-Layer Pattern

The Display project uses the same **single-layer architecture** as the LED example:

```
DisplayService
├── HttpEndpoint<DisplayState>      (REST API)
├── WebSocketTxRx<DisplayState>     (Real-time updates)
├── MqttPubSub<DisplayState>        (MQTT pub/sub)
├── BlePubSub<DisplayState>         (BLE read/write/notify)
└── LiquidCrystal_I2C              (Hardware driver)
```

### State Structure

```cpp
class DisplayState {
 public:
  String line1;           // Text for line 1 (max 16 chars)
  String line2;           // Text for line 2 (max 16 chars)
  uint8_t i2cAddress;     // I2C address (0x27 or 0x3F)
  bool backlight;         // Backlight on/off
};
```

**JSON representation**:
```json
{
  "line1": "Hello World",
  "line2": "Weighsoft",
  "i2c_address": 39,
  "backlight": true
}
```

> **Note**: `i2c_address` uses snake_case in JSON but camelCase (`i2cAddress`) in TypeScript.

### Multi-Channel Synchronization

All four channels share the same `DisplayState` and automatically synchronize. For example:

1. User types "Hello" via WebSocket UI
2. `WebSocketTxRx` calls `update(json, WS_ORIGIN_ID)`
3. State changes, `onConfigUpdated()` fires
4. LCD hardware updates to show "Hello"
5. `MqttPubSub` publishes new state to MQTT
6. `BlePubSub` notifies connected BLE clients

**No feedback loops**: Origin tracking ensures the originating channel doesn't re-broadcast.

## File Structure

### Backend (`src/examples/display/`)

| File | Purpose |
|------|---------|
| `DisplayService.h` | Service header with inline `DisplayState`, BLE UUIDs, endpoint paths |
| `DisplayService.cpp` | Service implementation: LCD init, state updates, MQTT/BLE config |

### Frontend (`interface/src/examples/display/`)

| File | Purpose |
|------|---------|
| `Display.tsx` | Main router with tabs: Info, Control, BLE |
| `DisplayInfo.tsx` | Hardware setup documentation and wiring guide |
| `DisplayControl.tsx` | Combined REST/WebSocket control with live preview |
| `DisplayBle.tsx` | BLE connection instructions and UUID reference |

### Shared Files

| File | Purpose |
|------|---------|
| `interface/src/types/display.ts` | TypeScript `DisplayData` interface |
| `interface/src/api/display.ts` | REST API functions (`readDisplayData`, `updateDisplayData`) |
| `interface/src/api/endpoints.ts` | `DISPLAY_SOCKET_PATH` WebSocket URL |

### Library

| File | Purpose |
|------|---------|
| `lib/LiquidCrystal_I2C/` | Vendored LCD library (marcoschwartz) with ESP32 platform patch |

> **Why vendored?** The upstream `library.json` only lists `atmelavr` and `espressif8266` as platforms. With `lib_compat_mode = strict`, PlatformIO rejects it for ESP32 despite the code working perfectly. The vendored copy adds `"espressif32"` to the platforms list.

## API Reference

### REST

**Read display state**:
```bash
curl http://<device-ip>/rest/display
```

Response:
```json
{
  "line1": "Weighsoft",
  "line2": "Display Ready",
  "i2c_address": 39,
  "backlight": true
}
```

**Update display state**:
```bash
curl -X POST http://<device-ip>/rest/display \
  -H "Content-Type: application/json" \
  -d '{"line1":"Hello","line2":"World","i2c_address":39,"backlight":true}'
```

### WebSocket

**Endpoint**: `ws://<device-ip>/ws/display`

**Security**: IS_AUTHENTICATED

Connect and receive current state, then send updates:
```javascript
const ws = new WebSocket('ws://192.168.3.124/ws/display');
ws.onmessage = (e) => console.log('Display:', JSON.parse(e.data));
ws.send(JSON.stringify({line1: "Hello", line2: "World", i2c_address: 39, backlight: true}));
```

### MQTT

**Topics** (where `{unique_id}` is derived from MAC address):

| Topic | Direction | Purpose |
|-------|-----------|---------|
| `weighsoft/display/{unique_id}/data` | Device publishes | Current state |
| `weighsoft/display/{unique_id}/set` | Device subscribes | Receive commands |

**Publish command**:
```bash
mosquitto_pub -h YOUR_MQTT_BROKER \
  -t "weighsoft/display/<unique_id>/set" \
  -m '{"line1":"Hello","line2":"MQTT"}'
```

**Subscribe to state**:
```bash
mosquitto_sub -h YOUR_MQTT_BROKER \
  -t "weighsoft/display/<unique_id>/data"
```

### BLE

**Platform**: ESP32 only

**Service & Characteristic UUIDs**:
- **Service UUID**: `a8f3d5e0-8b2c-4f1a-9d6e-3c7b4a5f1e8d`
- **Characteristic UUID**: `a8f3d5e1-8b2c-4f1a-9d6e-3c7b4a5f1e8d`

**Operations**: READ, WRITE, NOTIFY

**Payload format** (JSON):
```json
{"line1":"BLE Hello","line2":"World","i2c_address":39,"backlight":true}
```

**Using nRF Connect (Mobile)**:
1. Scan for "Weighsoft-*" device
2. Connect and find Display service (`a8f3d5e0-...`)
3. Write JSON to the characteristic
4. Enable notifications to receive state updates from other channels

## Use Cases

- **Status display**: Show system info, IP address, uptime
- **Notifications**: Display alerts from MQTT or REST
- **Sensor readings**: Show temperature, weight, or other sensor data
- **Debug output**: Display diagnostic messages during development
- **User messages**: Show custom messages from a dashboard or mobile app

## Troubleshooting

### Display Not Showing Text

1. **Check wiring**: Verify SDA (GPIO21), SCL (GPIO22), VCC (5V), GND connections
2. **Check I2C address**: Try switching between 0x27 and 0x3F in the UI
3. **Check contrast**: Adjust the potentiometer on the back of the LCD module
4. **Check power**: The LCD backpack needs 5V, not 3.3V

### Display Shows Garbled Characters

- Power cycle the device (unplug and replug)
- Check that SDA and SCL are not swapped
- Verify the I2C address matches your hardware

### WebSocket Not Connecting

- Check browser console for connection errors
- Verify the device IP address is correct
- Ensure you are authenticated (sign in first)

### MQTT Not Publishing

- Verify MQTT is enabled in the web UI (MQTT Settings)
- Check broker connection status
- Monitor serial output for MQTT connection messages

### BLE Not Available

- BLE is ESP32 only (not available on ESP8266)
- Enable BLE in the web UI (BLE Settings)
- Restart the device after enabling BLE

## See Also

- [LED-EXAMPLE.md](LED-EXAMPLE.md) - LED example (template this was based on)
- [DEVICE-TEMPLATE-GUIDE.md](DEVICE-TEMPLATE-GUIDE.md) - Guide for adding new devices
- [API-REFERENCE.md](API-REFERENCE.md) - Full REST and WebSocket API documentation
- [EXTENSION-GUIDE.md](EXTENSION-GUIDE.md) - Detailed service creation guide
