# C4 Model - Level 4: Code Patterns

## Overview

This document provides code-level diagrams and patterns (C4 Level 4) for the ESP8266-React framework. It shows implementation details, class structures, and detailed patterns that developers should follow when extending the framework.

## StatefulService Pattern

### Class Diagram

```mermaid
classDiagram
    class StatefulService~T~ {
        -T _state
        -list~StateUpdateHandlerInfo~ _updateHandlers
        -SemaphoreHandle_t _accessMutex (ESP32 only)
        +update(stateUpdater, originId) StateUpdateResult
        +updateWithoutPropagation(stateUpdater) StateUpdateResult
        +read(stateReader) void
        +addUpdateHandler(callback, allowRemove) update_handler_id_t
        +removeUpdateHandler(id) void
        +callUpdateHandlers(originId) void
        #beginTransaction() void
        #endTransaction() void
    }
    
    class StateUpdateHandlerInfo {
        +update_handler_id_t _id
        +StateUpdateCallback _cb
        +bool _allowRemove
    }
    
    class WiFiSettings {
        +String ssid
        +String password
        +String hostname
        +bool staticIPConfig
        +read(state, root) void$
        +update(root, state) StateUpdateResult$
    }
    
    class WiFiSettingsService {
        -HttpEndpoint~WiFiSettings~ _httpEndpoint
        -FSPersistence~WiFiSettings~ _fsPersistence
        +begin() void
        +loop() void
    }
    
    StatefulService~T~ <|-- WiFiSettingsService : inherits with T=WiFiSettings
    WiFiSettingsService ..> WiFiSettings : uses
    StatefulService~T~ o-- StateUpdateHandlerInfo : contains many
```

### Update Handler Flow

```mermaid
sequenceDiagram
    participant Client
    participant Service as StatefulService&lt;T&gt;
    participant Handler1 as FSPersistence
    participant Handler2 as WebSocketTx
    participant Handler3 as MqttPub
    
    Client->>Service: update(stateUpdater, "http")
    activate Service
    Service->>Service: beginTransaction()
    Service->>Service: stateUpdater(_state)
    Note over Service: State modified
    Service->>Service: endTransaction()
    
    alt StateUpdateResult::CHANGED
        Service->>Service: callUpdateHandlers("http")
        Service->>Handler1: callback("http")
        activate Handler1
        Handler1->>Handler1: writeToFS()
        Handler1-->>Service: done
        deactivate Handler1
        
        Service->>Handler2: callback("http")
        activate Handler2
        Handler2->>Handler2: transmitData(null, "http")
        Note over Handler2: Broadcast to WebSocket clients
        Handler2-->>Service: done
        deactivate Handler2
        
        Service->>Handler3: callback("http")
        activate Handler3
        Handler3->>Handler3: publish()
        Note over Handler3: Publish to MQTT broker
        Handler3-->>Service: done
        deactivate Handler3
    end
    
    Service-->>Client: StateUpdateResult::CHANGED
    deactivate Service
```

### Thread Safety (ESP32)

```mermaid
sequenceDiagram
    participant Thread1 as Thread 1
    participant Mutex as Recursive Mutex
    participant State as _state
    participant Thread2 as Thread 2
    
    Thread1->>Mutex: xSemaphoreTakeRecursive()
    activate Mutex
    Mutex-->>Thread1: acquired
    Thread1->>State: read/modify _state
    
    Thread2->>Mutex: xSemaphoreTakeRecursive()
    Note over Thread2: Blocked, waiting for mutex
    
    Thread1->>State: continue modifying
    Thread1->>Mutex: xSemaphoreGiveRecursive()
    deactivate Mutex
    
    Mutex-->>Thread2: acquired
    activate Mutex
    Thread2->>State: read/modify _state
    Thread2->>Mutex: xSemaphoreGiveRecursive()
    deactivate Mutex
```

## Service Composition Pattern

### WiFiSettingsService Composition

```mermaid
classDiagram
    class WiFiSettingsService {
        -WiFiSettings _state
        -HttpEndpoint~WiFiSettings~ _httpEndpoint
        -FSPersistence~WiFiSettings~ _fsPersistence
        -AsyncWebServer* _server
        -FS* _fs
        -SecurityManager* _securityManager
        +WiFiSettingsService(server, fs, securityManager)
        +begin() void
        +loop() void
        -reconfigureWiFiConnection() void
        -onStationModeDisconnected() void
        -onStationModeGotIP() void
    }
    
    class StatefulService~WiFiSettings~ {
        <<template>>
    }
    
    class HttpEndpoint~WiFiSettings~ {
        -JsonStateReader~WiFiSettings~ _stateReader
        -JsonStateUpdater~WiFiSettings~ _stateUpdater
        -StatefulService~WiFiSettings~* _statefulService
        +fetchSettings(request) void
        +updateSettings(request, json) void
    }
    
    class FSPersistence~WiFiSettings~ {
        -JsonStateReader~WiFiSettings~ _stateReader
        -JsonStateUpdater~WiFiSettings~ _stateUpdater
        -StatefulService~WiFiSettings~* _statefulService
        -FS* _fs
        -const char* _filePath
        +readFromFS() void
        +writeToFS() bool
    }
    
    class WiFiSettings {
        +String ssid
        +String password
        +String hostname
        +read(state, root)$ void
        +update(root, state)$ StateUpdateResult
    }
    
    WiFiSettingsService --|> StatefulService~WiFiSettings~ : inherits
    WiFiSettingsService *-- HttpEndpoint~WiFiSettings~ : composes
    WiFiSettingsService *-- FSPersistence~WiFiSettings~ : composes
    WiFiSettingsService ..> WiFiSettings : uses
    HttpEndpoint~WiFiSettings~ ..> WiFiSettings : serializes
    FSPersistence~WiFiSettings~ ..> WiFiSettings : persists
```

### Constructor Initialization Pattern

```mermaid
sequenceDiagram
    participant Main as main.cpp
    participant Service as WiFiSettingsService
    participant HTTP as HttpEndpoint
    participant FS as FSPersistence
    participant Stateful as StatefulService
    
    Main->>Service: WiFiSettingsService(server, fs, securityManager)
    activate Service
    
    Service->>HTTP: HttpEndpoint(read, update, this, server, path, security)
    activate HTTP
    HTTP->>HTTP: Register GET/POST handlers with server
    HTTP-->>Service: constructed
    deactivate HTTP
    
    Service->>FS: FSPersistence(read, update, this, fs, filepath)
    activate FS
    FS->>Stateful: addUpdateHandler(writeToFS callback)
    Note over FS: Auto-save on state changes
    FS-->>Service: constructed
    deactivate FS
    
    Service->>Stateful: addUpdateHandler(reconfigureWiFi callback)
    Note over Service: React to configuration changes
    
    Service-->>Main: constructed
    deactivate Service
    
    Main->>Service: begin()
    activate Service
    Service->>FS: readFromFS()
    activate FS
    FS->>FS: Load JSON from /config/wifiSettings.json
    FS->>Stateful: updateWithoutPropagation(parsed settings)
    FS-->>Service: loaded
    deactivate FS
    Service->>Service: reconfigureWiFiConnection()
    Service-->>Main: initialized
    deactivate Service
```

## Demo Light Service Pattern

### LightStateService Architecture

```mermaid
classDiagram
    class LightStateService {
        -LightState _state
        -HttpEndpoint~LightState~ _httpEndpoint
        -WebSocketTxRx~LightState~ _webSocket
        -MqttPubSub~LightState~ _mqttPubSub
        -LightMqttSettingsService* _lightMqttSettingsService
        -AsyncMqttClient* _mqttClient
        +LightStateService(server, securityManager, mqttClient, lightMqttSettingsService)
        +begin() void
        -registerConfig() void
        -onConfigUpdated(originId) void
    }
    
    class StatefulService~LightState~ {
        <<template>>
    }
    
    class HttpEndpoint~LightState~ {
        <<infrastructure>>
        +Exposes /rest/lightState
    }
    
    class WebSocketTxRx~LightState~ {
        <<infrastructure>>
        +Connects /ws/lightState
    }
    
    class MqttPubSub~LightState~ {
        <<infrastructure>>
        +pub: state topic
        +sub: set topic
    }
    
    class LightMqttSettingsService {
        -LightMqttSettings _state
        -HttpEndpoint~LightMqttSettings~ _httpEndpoint
        -FSPersistence~LightMqttSettings~ _fsPersistence
    }
    
    class LightState {
        +bool ledOn
        +read(state, root)$ void
        +update(root, state)$ StateUpdateResult
        +haRead(state, root)$ void
        +haUpdate(root, state)$ StateUpdateResult
    }
    
    class LightMqttSettings {
        +String mqttPath
        +String name
        +String uniqueId
        +read(state, root)$ void
        +update(root, state)$ StateUpdateResult
    }
    
    LightStateService --|> StatefulService~LightState~ : inherits
    LightStateService *-- HttpEndpoint~LightState~ : composes
    LightStateService *-- WebSocketTxRx~LightState~ : composes
    LightStateService *-- MqttPubSub~LightState~ : composes
    LightStateService ..> LightMqttSettingsService : observes
    LightStateService ..> LightState : uses
    LightMqttSettingsService ..> LightMqttSettings : uses
```

### Home Assistant Integration Flow

```mermaid
sequenceDiagram
    participant MQTT as MQTT Broker
    participant Service as LightStateService
    participant Settings as LightMqttSettingsService
    participant GPIO as LED GPIO
    
    Note over Service: MQTT client connects
    Service->>Settings: read(settings)
    Settings-->>Service: mqttPath, name, uniqueId
    
    Service->>MQTT: publish to {mqttPath}/config
    Note over MQTT: Home Assistant discovery
    
    Service->>MQTT: subscribe to {mqttPath}/set
    Note over Service: Ready to receive commands
    
    Service->>MQTT: publish to {mqttPath}/state
    Note over MQTT: Initial state {"state": "OFF"}
    
    Note over MQTT: User changes light in Home Assistant
    MQTT->>Service: message on {mqttPath}/set
    Note over Service: Payload: {"state": "ON"}
    
    Service->>Service: haUpdate(jsonObject)
    Note over Service: Parse "ON" → ledOn = true
    
    Service->>GPIO: digitalWrite(LED_PIN, HIGH)
    Note over GPIO: LED turns on
    
    Service->>Service: callUpdateHandlers("mqtt")
    
    Service->>MQTT: publish to {mqttPath}/state
    Note over MQTT: {"state": "ON"}
    
    Service->>Service: WebSocketTx broadcasts
    Note over Service: Update all connected web clients
```

## Serialization Pattern

### JsonStateReader and JsonStateUpdater

```mermaid
classDiagram
    class WiFiSettings {
        +String ssid
        +String password
        +String hostname
        +bool staticIPConfig
        +IPAddress localIP
        +IPAddress gatewayIP
        +IPAddress subnetMask
        +IPAddress dnsIP1
        +IPAddress dnsIP2
    }
    
    class JsonStateReader {
        <<function type>>
        +void(WiFiSettings&, JsonObject&)
    }
    
    class JsonStateUpdater {
        <<function type>>
        +StateUpdateResult(JsonObject&, WiFiSettings&)
    }
    
    WiFiSettings ..> JsonStateReader : read static method
    WiFiSettings ..> JsonStateUpdater : update static method
    
    note for JsonStateReader "Serializes state to JSON:\nroot['ssid'] = state.ssid;\nroot['password'] = state.password;\n..."
    
    note for JsonStateUpdater "Deserializes JSON to state:\nstate.ssid = root['ssid'] | '';\nstate.password = root['password'] | '';\nreturn StateUpdateResult::CHANGED;"
```

### WiFiSettings Serialization Example

```cpp
// JsonStateReader implementation
static void read(WiFiSettings& settings, JsonObject& root) {
    root["ssid"] = settings.ssid;
    root["password"] = settings.password;
    root["hostname"] = settings.hostname;
    root["static_ip_config"] = settings.staticIPConfig;
    
    if (settings.staticIPConfig) {
        root["local_ip"] = settings.localIP.toString();
        root["gateway_ip"] = settings.gatewayIP.toString();
        root["subnet_mask"] = settings.subnetMask.toString();
        root["dns_ip_1"] = settings.dnsIP1.toString();
        root["dns_ip_2"] = settings.dnsIP2.toString();
    }
}

// JsonStateUpdater implementation
static StateUpdateResult update(JsonObject& root, WiFiSettings& settings) {
    settings.ssid = root["ssid"] | "";
    settings.password = root["password"] | "";
    settings.hostname = root["hostname"] | SettingValue::format("#{platform}-#{unique_id}");
    settings.staticIPConfig = root["static_ip_config"] | false;
    
    if (settings.staticIPConfig) {
        settings.localIP.fromString(root["local_ip"] | "192.168.1.100");
        settings.gatewayIP.fromString(root["gateway_ip"] | "192.168.1.1");
        settings.subnetMask.fromString(root["subnet_mask"] | "255.255.255.0");
        settings.dnsIP1.fromString(root["dns_ip_1"] | "1.1.1.1");
        settings.dnsIP2.fromString(root["dns_ip_2"] | "8.8.8.8");
    }
    
    return StateUpdateResult::CHANGED;
}
```

## Origin Tracking Pattern

### Circular Update Prevention

```mermaid
flowchart TB
    Start[HTTP POST /rest/lightState] --> UpdateState[Update state with origin 'http']
    UpdateState --> CallHandlers[Call update handlers]
    
    CallHandlers --> FSPersist[FSPersistence handler]
    FSPersist --> WriteFile[Write to filesystem]
    
    CallHandlers --> WSHandler[WebSocketTx handler]
    WSHandler --> CheckOrigin{origin_id == 'websocket:{id}' ?}
    CheckOrigin -->|Yes| SkipWS[Skip this client]
    CheckOrigin -->|No| BroadcastWS[Broadcast to client]
    
    CallHandlers --> MQTTHandler[MqttPub handler]
    MQTTHandler --> Publish[Publish to MQTT broker]
    
    Publish --> ExternalMQTT[External system receives state]
    ExternalMQTT --> NoLoop[Does NOT send command back]
    
    WriteFile --> End[Done]
    SkipWS --> End
    BroadcastWS --> End
    
    style Start fill:#4fc3f7
    style CheckOrigin fill:#fff59d
    style SkipWS fill:#ffccbc
    style NoLoop fill:#c8e6c9
```

### Origin ID Usage

| Origin ID | Source | Propagation Behavior |
|-----------|--------|---------------------|
| `"http"` | REST API POST | Broadcast to WS, publish to MQTT, save to FS |
| `"mqtt"` | MQTT message | Broadcast to WS, save to FS (if configured), do NOT publish back to MQTT |
| `"websocket:{id}"` | WebSocket client | Broadcast to other WS clients (skip origin), publish to MQTT, save to FS |
| `"internal"` | Code initialization | Broadcast to all, publish to MQTT, save to FS |

## Infrastructure Component Patterns

### HttpEndpoint Pattern

```mermaid
sequenceDiagram
    participant Client as HTTP Client
    participant Server as AsyncWebServer
    participant Endpoint as HttpEndpoint&lt;T&gt;
    participant Security as SecurityManager
    participant Service as StatefulService&lt;T&gt;
    participant FS as Filesystem
    
    Note over Client,FS: GET Request Flow
    Client->>Server: GET /rest/wifiSettings
    Server->>Endpoint: fetchSettings(request)
    Endpoint->>Security: wrapRequest validates JWT
    alt Authorized
        Endpoint->>Service: read(jsonObject, stateReader)
        Service->>Service: _stateReader(_state, jsonObject)
        Service-->>Endpoint: JSON populated
        Endpoint-->>Client: 200 OK with JSON
    else Unauthorized
        Security-->>Client: 401 Unauthorized
    end
    
    Note over Client,FS: POST Request Flow
    Client->>Server: POST /rest/wifiSettings with JSON
    Server->>Endpoint: updateSettings(request, json)
    Endpoint->>Security: wrapCallback validates JWT
    alt Authorized
        Endpoint->>Service: updateWithoutPropagation(json, stateUpdater)
        Service->>Service: _stateUpdater(json, _state)
        Service-->>Endpoint: StateUpdateResult::CHANGED
        
        alt CHANGED
            Endpoint->>Endpoint: onDisconnect callback registered
            Note over Endpoint: Waits for response to be sent
            Endpoint-->>Client: 200 OK with updated JSON
            Note over Endpoint: Connection closed
            Endpoint->>Service: callUpdateHandlers("http")
            Service->>FS: FSPersistence writes to file
        else UNCHANGED
            Endpoint-->>Client: 200 OK with JSON
        else ERROR
            Endpoint-->>Client: 400 Bad Request
        end
    else Unauthorized
        Security-->>Client: 401 Unauthorized
    end
```

### FSPersistence Pattern

```mermaid
flowchart TB
    ReadStart[Service begin calls readFromFS] --> OpenFile{File exists?}
    OpenFile -->|Yes| ParseJSON[Parse JSON file]
    OpenFile -->|No| UseDefaults[Use factory defaults]
    
    ParseJSON --> ParseOK{Parse successful?}
    ParseOK -->|Yes| Deserialize[Call stateUpdater with JSON]
    ParseOK -->|No| UseDefaults
    
    Deserialize --> UpdateState[updateWithoutPropagation]
    UseDefaults --> UpdateState
    UpdateState --> ReadDone[Ready]
    
    WriteStart[State change occurs] --> HandlerTriggered[Update handler triggered]
    HandlerTriggered --> SerializeState[Serialize state to JSON]
    SerializeState --> MkDirs[Create directories if needed]
    MkDirs --> OpenForWrite[Open file for writing]
    OpenForWrite --> WriteJSON[Write JSON to file]
    WriteJSON --> CloseFile[Close file]
    CloseFile --> WriteDone[Persisted]
    
    style ReadStart fill:#e1f5fe
    style WriteStart fill:#fff3e0
    style ReadDone fill:#c8e6c9
    style WriteDone fill:#c8e6c9
```

### MqttPubSub Pattern

```mermaid
sequenceDiagram
    participant Service as StatefulService&lt;T&gt;
    participant PubSub as MqttPubSub&lt;T&gt;
    participant Client as AsyncMqttClient
    participant Broker as MQTT Broker
    
    Note over Client,Broker: Connection Established
    Client->>PubSub: onConnect callback
    PubSub->>Client: subscribe(subTopic)
    PubSub->>Service: read state
    PubSub->>Client: publish(pubTopic, state)
    Client->>Broker: PUBLISH to pubTopic
    
    Note over Service,Broker: State Update (Internal)
    Service->>Service: State changes internally
    Service->>PubSub: Update handler triggered
    PubSub->>Service: read(jsonObject, stateReader)
    PubSub->>Client: publish(pubTopic, JSON)
    Client->>Broker: PUBLISH to pubTopic
    
    Note over Service,Broker: External Command
    Broker->>Client: PUBLISH to subTopic
    Client->>PubSub: onMessage(topic, payload)
    PubSub->>PubSub: Parse JSON payload
    PubSub->>Service: update(jsonObject, stateUpdater, "mqtt")
    Service->>Service: State updated
    Service->>PubSub: Update handler (different one)
    Note over PubSub: Does NOT publish back (origin="mqtt")
```

### WebSocketTxRx Pattern

```mermaid
sequenceDiagram
    participant Client as Browser WebSocket
    participant Server as AsyncWebSocket
    participant TxRx as WebSocketTxRx&lt;T&gt;
    participant Service as StatefulService&lt;T&gt;
    
    Note over Client,Service: Connection Flow
    Client->>Server: WebSocket upgrade
    Server->>TxRx: onWSEvent(WS_EVT_CONNECT)
    TxRx->>Client: Send client ID {"type":"id","id":"websocket:123"}
    TxRx->>Service: read current state
    TxRx->>Client: Send payload {"type":"payload","payload":{...}}
    
    Note over Client,Service: Client Sends Update
    Client->>Server: Send JSON message
    Server->>TxRx: onWSEvent(WS_EVT_DATA)
    TxRx->>TxRx: Parse JSON
    TxRx->>Service: update(json, stateUpdater, "websocket:123")
    Service->>Service: State updated
    Service->>TxRx: Update handler triggered
    TxRx->>TxRx: Check origin != "websocket:123"
    TxRx->>Server: Broadcast to OTHER clients
    Server->>Client: Other clients receive update
    
    Note over Client,Service: External Update (HTTP)
    Service->>Service: HTTP POST updates state
    Service->>TxRx: Update handler ("http" origin)
    TxRx->>Server: Broadcast to ALL clients
    Server->>Client: All clients receive update
```

## Best Practices

### 1. State Update Results

Always return appropriate result:

```cpp
StateUpdateResult update(JsonObject& root, Settings& settings) {
    bool changed = false;
    
    String newSSID = root["ssid"] | "";
    if (settings.ssid != newSSID) {
        settings.ssid = newSSID;
        changed = true;
    }
    
    return changed ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
}
```

### 2. Update Handler Registration

Register handlers with appropriate `allowRemove` flag:

```cpp
// Infrastructure handlers: allowRemove = false (permanent)
_statefulService->addUpdateHandler([&](const String& originId) { 
    writeToFS(); 
}, false);

// User handlers: allowRemove = true (default)
_statefulService->addUpdateHandler([&](const String& originId) { 
    reconfigureHardware(); 
});
```

### 3. Origin Checking

Check origin before acting:

```cpp
void onUpdate(const String& originId) {
    // Don't re-publish if origin is MQTT
    if (originId != "mqtt") {
        _mqttClient->publish(topic, payload);
    }
}
```

### 4. Thread-Safe State Access

Always use read/update methods (never access `_state` directly):

```cpp
// CORRECT
_statefulService->read([&](Settings& settings) {
    digitalWrite(LED_PIN, settings.ledOn ? HIGH : LOW);
});

// WRONG (not thread-safe on ESP32)
digitalWrite(LED_PIN, _statefulService->_state.ledOn ? HIGH : LOW);
```

### 5. Composition Over Inheritance

Compose infrastructure components:

```cpp
class MyService : public StatefulService<MySettings> {
    HttpEndpoint<MySettings> _httpEndpoint;      // REST API
    FSPersistence<MySettings> _fsPersistence;    // Persistence
    WebSocketTxRx<MySettings> _webSocket;        // Real-time
    MqttPubSub<MySettings> _mqttPubSub;          // MQTT
};
```

## Next Steps

- [SEQUENCE-DIAGRAMS.md](SEQUENCE-DIAGRAMS.md) - Detailed interaction flows
- [DESIGN-PATTERNS.md](DESIGN-PATTERNS.md) - Complete pattern catalog
- [EXTENSION-GUIDE.md](EXTENSION-GUIDE.md) - Implement these patterns
