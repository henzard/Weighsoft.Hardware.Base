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

## Serial Bridge Feature

### Overview

The Display device can connect to a Serial device (running on a separate ESP32 from the 'serial' branch) and display streaming serial data on the LCD. Three connection modes are supported:

1. **WebSocket Mode**: Direct device-to-device connection
2. **MQTT Mode**: Pub/sub via MQTT broker
3. **BLE Mode**: Bluetooth connection (no WiFi required)

### Architecture

#### WebSocket Mode
```
Serial Device ESP32 → WebSocket Server (/ws/serial)
                          ↓
Display Device ESP32 → WebSocket Client → LCD Screen
```

#### MQTT Mode
```
Serial Device ESP32 → Publish to weighsoft/serial/{id}/data
                          ↓
                     MQTT Broker
                          ↓
Display Device ESP32 → Subscribe to topic → LCD Screen
```

#### BLE Mode
```
Serial Device ESP32 → BLE Server (Serial Service)
                          ↓
Display Device ESP32 → BLE Client (scan & subscribe) → LCD Screen
```

### Setup Instructions

#### Prerequisites
1. Deploy Serial device (from 'serial' branch) on ESP32 #1
2. Deploy Display device (from 'display' branch) on ESP32 #2
3. For WebSocket: Both devices on same WiFi network
4. For MQTT: Both devices connected to same MQTT broker
5. For BLE: Devices within Bluetooth range (~10-30 meters)

#### WebSocket Mode Setup
1. Note the Serial device's IP address (e.g., 192.168.3.124)
2. Open Display device web UI
3. Navigate to "Display" → "Serial Bridge" tab
4. Select "WebSocket (Direct Connection)" mode
5. Enter Serial device IP and port (usually 80)
6. Click "Apply Configuration"
7. Serial data will stream to LCD in real-time

#### MQTT Mode Setup
1. Open Serial device web UI and find MQTT topic (e.g., `weighsoft/serial/a4e57cdb7928/data`)
2. Ensure MQTT broker is configured and connected on both devices
3. Open Display device web UI
4. Navigate to "Display" → "Serial Bridge" tab
5. Select "MQTT (Pub/Sub via Broker)" mode
6. Enter Serial device's MQTT topic
7. Click "Apply Configuration"
8. Serial data will stream to LCD via MQTT

#### BLE Mode Setup
1. Open Serial device web UI and find BLE UUIDs under the BLE tab
   - Service UUID (e.g., `0000181a-0000-1000-8000-00805f9b34fb`)
   - Characteristic UUID (e.g., `00002a58-0000-1000-8000-00805f9b34fb`)
2. Open Display device web UI
3. Navigate to "Display" → "Serial Bridge" tab
4. Select "BLE (Bluetooth - No WiFi)" mode
5. Enter Serial device's BLE Service UUID and Characteristic UUID
6. Click "Apply Configuration"
7. Display will scan for Serial device and connect automatically
8. Serial data will stream to LCD via BLE

### Configuration

#### Bridge Mode Options
- **off**: Bridge disabled, Display operates normally
- **websocket**: WebSocket client mode
- **mqtt**: MQTT subscription mode
- **ble**: BLE client mode

#### WebSocket Mode Settings
- **Serial Device IP**: IP address of Serial device (e.g., 192.168.3.100)
- **Port**: Usually 80 (HTTP)

#### MQTT Mode Settings
- **Serial MQTT Topic**: Full topic path (e.g., `weighsoft/serial/a4e57cdb7928/data`)

#### BLE Mode Settings
- **Serial BLE Service UUID**: Service UUID from Serial device
- **Serial BLE Characteristic UUID**: Characteristic UUID from Serial device

### Display Format

Serial data is displayed on the 2x16 LCD:
- Lines up to 16 chars: Displayed on Row 1, Row 2 padded with spaces
- Lines 17-32 chars: Split across Row 1 (chars 1-16) and Row 2 (chars 17-32)
- Lines over 32 chars: Truncated to 32 chars

### Use Cases

1. **Remote Serial Monitor**
   - Monitor serial output from distant device
   - No need for physical connection or USB cable

2. **Wireless Serial-to-LCD Adapter**
   - Convert any serial stream to LCD display
   - Useful for embedded systems without displays

3. **Multi-Device Data Visualization**
   - Multiple displays can subscribe to same MQTT topic
   - Distribute serial data to several locations

4. **Industrial Monitoring**
   - Display sensor readings from remote devices
   - MQTT mode provides reliable delivery

5. **Offline/Battery Operation**
   - Use BLE mode when WiFi is unavailable
   - Lower power consumption than WiFi

### Comparison: WebSocket vs MQTT vs BLE

| Feature | WebSocket Mode | MQTT Mode | BLE Mode |
|---------|---------------|-----------|----------|
| Latency | Lowest (~50ms) | Medium (100-500ms) | Low (~100ms) |
| Reliability | Depends on WiFi | Broker handles reconnect | Good within range |
| Configuration | Needs IP address | Needs topic name | Needs BLE UUIDs |
| Scalability | 1:1 connection | Many subscribers | 1:1 connection |
| Network | Same WiFi required | Broker required | No WiFi needed |
| Range | WiFi range | Unlimited (via broker) | 10-30 meters |
| Best For | Single display, low latency | Multiple displays, reliability | Offline, battery-powered |

### Troubleshooting

#### WebSocket Mode Issues

**Problem**: "WebSocket bridge disconnected" in serial monitor

**Solutions**:
1. Verify Serial device IP is correct
2. Check both devices on same WiFi network
3. Ensure Serial device is powered on
4. Check firewall settings

**Problem**: No data on LCD

**Solutions**:
1. Verify Serial device is receiving serial data
2. Check WebSocket connection in serial monitor
3. Ensure bridge mode is "websocket"
4. Try restarting Display device

#### MQTT Mode Issues

**Problem**: No data on LCD

**Solutions**:
1. Verify MQTT broker is running
2. Check both devices connected to MQTT (check MQTT tab)
3. Verify MQTT topic name is correct (copy from Serial device)
4. Check MQTT connection status in serial monitor

**Problem**: Delayed updates

**Solutions**:
1. Normal for MQTT mode (slight delay expected)
2. Check MQTT broker performance
3. Consider WebSocket mode for lower latency

#### BLE Mode Issues

**Problem**: "Serial device not found via BLE scan" in serial monitor

**Solutions**:
1. Verify Serial device has BLE enabled and advertising
2. Check devices are within Bluetooth range (10-30 meters)
3. Verify BLE UUIDs are correct (copy from Serial device BLE tab)
4. Try restarting Display device to re-scan

**Problem**: "Failed to find service" or "Failed to find characteristic"

**Solutions**:
1. Verify the BLE UUIDs exactly match Serial device
2. Ensure Serial device is advertising the service
3. Check serial output for detailed error messages
4. Try re-enabling BLE on Serial device

**Problem**: BLE connection drops

**Solutions**:
1. Move devices closer together
2. Check for interference (other BLE devices, microwaves)
3. Consider using WebSocket or MQTT for longer distances

### Technical Details

#### Data Format

Serial data is transmitted in JSON format across all modes:

```json
{
  "last_line": "Temperature: 23.5C",
  "timestamp": 1234567890
}
```

#### Connection Management

- **WebSocket**: Automatic reconnection with exponential backoff
- **MQTT**: Managed by AsyncMqttClient library
- **BLE**: Automatic scanning and connection on mode activation
- **State Persistence**: Bridge configuration saved to filesystem

#### Performance

- **WebSocket Mode**: < 50ms latency typical
- **MQTT Mode**: 100-500ms latency typical (depends on broker)
- **BLE Mode**: ~100ms latency typical
- **CPU Usage**: < 5% additional load per mode
- **Memory**: ~4KB overhead for WebSocket client, ~2KB for BLE client

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
