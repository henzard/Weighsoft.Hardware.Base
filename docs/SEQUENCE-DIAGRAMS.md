# Sequence Diagrams

## Overview

This document provides detailed sequence diagrams showing the interaction flows between components in the ESP8266-React framework. These diagrams illustrate how components collaborate to accomplish specific tasks.

## System Initialization

### Main Initialization Sequence

```mermaid
sequenceDiagram
    participant Main as main.cpp
    participant ESP8266React as ESP8266React
    participant Services as Feature Services
    participant FS as LittleFS
    participant WiFi as WiFi Stack
    participant Server as AsyncWebServer
    
    Note over Main: Power on / Reset
    Main->>Main: Serial.begin(115200)
    
    Main->>ESP8266React: esp8266React.begin()
    activate ESP8266React
    
    ESP8266React->>FS: ESPFS.begin()
    activate FS
    FS->>FS: Mount LittleFS filesystem
    FS-->>ESP8266React: Mounted
    deactivate FS
    
    ESP8266React->>Services: wifiSettingsService.begin()
    activate Services
    Services->>FS: FSPersistence.readFromFS()
    FS-->>Services: WiFiSettings loaded
    Services->>WiFi: Connect to network
    Services-->>ESP8266React: Initialized
    deactivate Services
    
    ESP8266React->>Services: apSettingsService.begin()
    activate Services
    Services->>FS: FSPersistence.readFromFS()
    FS-->>Services: APSettings loaded
    Services-->>ESP8266React: Initialized
    deactivate Services
    
    ESP8266React->>Services: ntpSettingsService.begin()
    ESP8266React->>Services: mqttSettingsService.begin()
    ESP8266React->>Services: securitySettingsService.begin()
    Note over Services: All services load config from filesystem
    
    ESP8266React-->>Main: Framework initialized
    deactivate ESP8266React
    
    Main->>Services: lightStateService.begin()
    Main->>Services: lightMqttSettingsService.begin()
    
    Main->>Server: server.begin()
    activate Server
    Server->>Server: Start listening on port 80
    Server-->>Main: Server started
    deactivate Server
    
    Note over Main: Enter main loop
    loop Forever
        Main->>ESP8266React: esp8266React.loop()
        ESP8266React->>Services: WiFi, AP, MQTT, OTA loop()
    end
```

### Service Registration Sequence

```mermaid
sequenceDiagram
    participant Constructor as Service Constructor
    participant Service as StatefulService&lt;T&gt;
    participant HTTP as HttpEndpoint
    participant WS as WebSocketTxRx
    participant FSP as FSPersistence
    participant MQTT as MqttPubSub
    participant Server as AsyncWebServer
    
    Constructor->>HTTP: new HttpEndpoint(read, update, this, server, path, security)
    activate HTTP
    HTTP->>Server: server->on(path, HTTP_GET, handler)
    HTTP->>Server: server->addHandler(postHandler)
    HTTP-->>Constructor: Registered GET/POST handlers
    deactivate HTTP
    
    Constructor->>WS: new WebSocketTxRx(read, update, this, server, wsPath, security)
    activate WS
    WS->>Server: server->addHandler(&_webSocket)
    WS->>Service: addUpdateHandler(broadcast callback)
    WS-->>Constructor: WebSocket registered
    deactivate WS
    
    Constructor->>FSP: new FSPersistence(read, update, this, fs, filePath)
    activate FSP
    FSP->>Service: addUpdateHandler(writeToFS callback)
    FSP-->>Constructor: Auto-save enabled
    deactivate FSP
    
    Constructor->>MQTT: new MqttPubSub(read, update, this, mqttClient, pubTopic, subTopic)
    activate MQTT
    MQTT->>Service: addUpdateHandler(publish callback)
    MQTT->>MQTT: mqttClient->onConnect(subscribe callback)
    MQTT->>MQTT: mqttClient->onMessage(receive callback)
    MQTT-->>Constructor: MQTT pub/sub configured
    deactivate MQTT
```

## State Update Flows

### REST API Update Flow

```mermaid
sequenceDiagram
    participant Browser as Web Browser
    participant Server as AsyncWebServer
    participant HTTP as HttpEndpoint
    participant Security as SecurityManager
    participant Service as StatefulService
    participant FS as FSPersistence
    participant WS as WebSocketTx
    participant MQTT as MqttPub
    participant Clients as Other WebSocket Clients
    
    Browser->>Server: POST /rest/wifiSettings<br/>{ssid: "NewWiFi", password: "secret"}
    Server->>HTTP: updateSettings(request, json)
    
    HTTP->>Security: wrapCallback checks JWT
    activate Security
    Security->>Security: Verify token signature
    Security->>Security: Check expiration
    Security-->>HTTP: Authorized (admin)
    deactivate Security
    
    HTTP->>Service: updateWithoutPropagation(json, stateUpdater)
    activate Service
    Service->>Service: beginTransaction() [Lock mutex on ESP32]
    Service->>Service: stateUpdater(json, _state)
    Note over Service: ssid = "NewWiFi"<br/>password = "secret"
    Service->>Service: endTransaction() [Unlock mutex]
    Service-->>HTTP: StateUpdateResult::CHANGED
    deactivate Service
    
    HTTP->>HTTP: Register onDisconnect callback
    HTTP->>Service: read(json, stateReader)
    HTTP-->>Browser: 200 OK {ssid: "NewWiFi", ...}
    
    Note over HTTP,Browser: Response sent, connection closed
    HTTP->>Service: callUpdateHandlers("http")
    activate Service
    
    par Parallel Handler Execution
        Service->>FS: handler callback("http")
        activate FS
        FS->>FS: Serialize state to JSON
        FS->>FS: Write to /config/wifiSettings.json
        FS-->>Service: Saved
        deactivate FS
    and
        Service->>WS: handler callback("http")
        activate WS
        WS->>WS: Serialize state to JSON
        WS->>Clients: Broadcast {"type":"payload", "origin_id":"http", ...}
        WS-->>Service: Broadcasted
        deactivate WS
    and
        Service->>MQTT: handler callback("http")
        activate MQTT
        MQTT->>MQTT: Serialize state to JSON
        MQTT->>MQTT: publish(topic, payload)
        MQTT-->>Service: Published
        deactivate MQTT
    end
    
    deactivate Service
```

### WebSocket Bidirectional Flow

```mermaid
sequenceDiagram
    participant Browser as Web Browser
    participant Server as AsyncWebSocket
    participant WS as WebSocketTxRx
    participant Service as StatefulService
    participant HTTP as (via HttpEndpoint)
    participant Other as Other Clients
    
    Note over Browser,Server: Initial Connection
    Browser->>Server: WebSocket Upgrade Request
    Server->>WS: onWSEvent(WS_EVT_CONNECT, client)
    WS->>Browser: Send {"type":"id", "id":"websocket:12345"}
    WS->>Service: read current state
    WS->>Browser: Send {"type":"payload", "payload":{...}}
    
    Note over Browser,Other: Client Sends Update
    Browser->>Server: Send JSON {led_on: true}
    Server->>WS: onWSEvent(WS_EVT_DATA)
    WS->>WS: Parse JSON
    WS->>Service: update(json, stateUpdater, "websocket:12345")
    activate Service
    Service->>Service: State updated: led_on = true
    Service->>Service: callUpdateHandlers("websocket:12345")
    
    Service->>WS: handler callback("websocket:12345")
    WS->>WS: Check if origin == "websocket:12345"
    WS->>Other: Broadcast to OTHER clients only
    Note over Other: Clients except 12345 receive update
    
    Service->>HTTP: FSPersistence writes to file
    Service->>HTTP: MqttPub publishes to broker
    deactivate Service
    
    Note over Browser,Other: External Update (from HTTP)
    HTTP->>Service: State updated via REST (origin: "http")
    activate Service
    Service->>WS: handler callback("http")
    WS->>Browser: Send to ALL clients (including 12345)
    WS->>Other: Send to all other clients
    deactivate Service
```

### MQTT Integration Flow

```mermaid
sequenceDiagram
    participant HA as Home Assistant
    participant Broker as MQTT Broker
    participant Client as AsyncMqttClient
    participant MQTT as MqttPubSub
    participant Service as StatefulService
    participant GPIO as Hardware (LED)
    participant WS as WebSocket Clients
    
    Note over Client,Broker: Connection Establishment
    Client->>Broker: CONNECT (client_id, username, password)
    Broker-->>Client: CONNACK
    Client->>MQTT: onConnect callback
    
    MQTT->>Client: subscribe("homeassistant/light/esp_light/set", QoS 2)
    Client->>Broker: SUBSCRIBE
    Broker-->>Client: SUBACK
    
    MQTT->>Service: read current state
    MQTT->>Client: publish("homeassistant/light/esp_light/state", {"state":"OFF"})
    Client->>Broker: PUBLISH (state topic)
    
    MQTT->>Client: publish("homeassistant/light/esp_light/config", {...})
    Client->>Broker: PUBLISH (discovery config)
    Note over Broker,HA: Home Assistant discovers device
    
    Note over HA,GPIO: User Commands from Home Assistant
    HA->>Broker: PUBLISH to set topic {"state":"ON"}
    Broker->>Client: PUBLISH message
    Client->>MQTT: onMessage("homeassistant/light/esp_light/set", {"state":"ON"})
    
    MQTT->>MQTT: Parse JSON: state="ON" → led_on=true
    MQTT->>Service: update(json, haUpdate, "mqtt")
    activate Service
    Service->>Service: State updated: led_on = true
    Service->>Service: callUpdateHandlers("mqtt")
    
    Service->>GPIO: Update handler: digitalWrite(LED_PIN, HIGH)
    GPIO->>GPIO: LED turns ON
    
    Service->>MQTT: handler callback("mqtt")
    MQTT->>MQTT: Check origin != "mqtt", skip publish
    Note over MQTT: Prevents echo back to MQTT
    
    Service->>WS: WebSocketTx broadcasts to browsers
    
    Service->>MQTT: (Another handler for state confirmation)
    MQTT->>Client: publish(state topic, {"state":"ON"})
    Client->>Broker: PUBLISH
    Broker->>HA: State confirmation
    deactivate Service
```

## Authentication Flow

### Sign In Sequence

```mermaid
sequenceDiagram
    participant Browser as Web Browser
    participant Server as AsyncWebServer
    participant AuthSvc as AuthenticationService
    participant SecMgr as SecuritySettingsService
    participant JWT as JWT Library
    participant Storage as localStorage/sessionStorage
    
    Browser->>Browser: User enters credentials
    Browser->>Server: POST /rest/signIn<br/>{username: "admin", password: "admin"}
    Server->>AuthSvc: signIn(request, json)
    
    AuthSvc->>SecMgr: read security settings
    SecMgr-->>AuthSvc: users list, jwt_secret
    
    AuthSvc->>AuthSvc: Find user by username
    
    alt User Found
        AuthSvc->>AuthSvc: Verify password hash
        
        alt Password Valid
            AuthSvc->>JWT: Generate token
            activate JWT
            Note over JWT: Payload:<br/>{username, admin, exp}
            JWT->>JWT: Sign with jwt_secret
            JWT-->>AuthSvc: JWT token
            deactivate JWT
            
            AuthSvc-->>Browser: 200 OK<br/>{access_token, username, admin}
            
            Browser->>Storage: Store token
            Note over Storage: localStorage (remember me)<br/>or sessionStorage
            
            Browser->>Browser: Update AuthContext
            Browser->>Browser: Navigate to dashboard
        else Password Invalid
            AuthSvc-->>Browser: 401 Unauthorized<br/>"Invalid credentials"
        end
    else User Not Found
        AuthSvc-->>Browser: 401 Unauthorized<br/>"Invalid credentials"
    end
```

### Authenticated Request Flow

```mermaid
sequenceDiagram
    participant Browser as Web Browser
    participant Axios as Axios Interceptor
    participant Storage as Token Storage
    participant Server as AsyncWebServer
    participant SecMgr as SecurityManager
    participant JWT as JWT Validation
    participant Handler as Request Handler
    
    Browser->>Axios: GET /rest/securitySettings
    Axios->>Storage: getToken()
    Storage-->>Axios: "eyJ0eXAi..."
    
    Axios->>Axios: Add Authorization header:<br/>"Bearer eyJ0eXAi..."
    Axios->>Server: GET with Authorization header
    
    Server->>SecMgr: wrapRequest checks predicate
    SecMgr->>SecMgr: Extract token from header
    
    SecMgr->>JWT: Decode and verify token
    activate JWT
    JWT->>JWT: Verify signature with jwt_secret
    JWT->>JWT: Check expiration
    
    alt Token Valid
        JWT-->>SecMgr: Decoded payload {username, admin}
        deactivate JWT
        
        SecMgr->>SecMgr: Check predicate (IS_ADMIN)
        
        alt User is Admin
            SecMgr->>Handler: Execute handler
            Handler->>Handler: Process request
            Handler-->>Browser: 200 OK with data
        else User Not Admin
            SecMgr-->>Browser: 403 Forbidden
        end
    else Token Invalid or Expired
        JWT-->>SecMgr: Verification failed
        deactivate JWT
        SecMgr-->>Browser: 401 Unauthorized
        
        Browser->>Browser: Clear token from storage
        Browser->>Browser: Redirect to /sign-in
    end
```

### WebSocket Authentication

```mermaid
sequenceDiagram
    participant Browser as Web Browser
    participant Server as AsyncWebSocket
    participant WS as WebSocketTxRx
    participant Security as SecurityManager
    participant Filter as WebSocket Filter
    
    Browser->>Server: WebSocket Upgrade Request
    Note over Browser: May include token in URL or headers
    
    Server->>WS: onWSEvent (before connection)
    WS->>Security: filterRequest(predicate)
    Security->>Filter: Apply authentication filter
    
    Filter->>Filter: Extract credentials
    Filter->>Filter: Verify JWT token
    
    alt Authorized
        Filter-->>Security: Allow connection
        Security-->>WS: Connection authorized
        WS->>Browser: Connection established
        WS->>Browser: Send client ID
        WS->>Browser: Send initial state
    else Unauthorized
        Filter-->>Security: Reject connection
        Security-->>Browser: 403 Forbidden
        Note over Browser: WebSocket connection refused
    end
```

## MQTT Home Assistant Discovery

### Discovery and State Synchronization

```mermaid
sequenceDiagram
    participant Device as ESP8266 Device
    participant Light as LightStateService
    participant Settings as LightMqttSettingsService
    participant MQTT as MqttPubSub
    participant Broker as MQTT Broker
    participant HA as Home Assistant
    
    Note over Device,HA: Device Boots and Connects
    Device->>MQTT: MQTT client connects
    MQTT->>Light: onConnect callback
    
    Light->>Settings: read MQTT settings
    Settings-->>Light: mqttPath, name, uniqueId
    
    Light->>Light: Build discovery payload
    Note over Light: {<br/>  name: "ESP Light",<br/>  unique_id: "light-abc123",<br/>  cmd_t: "path/set",<br/>  stat_t: "path/state",<br/>  schema: "json"<br/>}
    
    Light->>MQTT: Publish discovery config
    MQTT->>Broker: PUBLISH to homeassistant/light/esp_light/config
    Broker->>HA: Discovery message
    HA->>HA: Register new light entity
    Note over HA: Light appears in UI
    
    Light->>MQTT: Publish initial state
    MQTT->>Broker: PUBLISH to path/state {"state":"OFF"}
    Broker->>HA: Initial state
    HA->>HA: Update entity state
    
    Note over Device,HA: User Controls Light from HA
    HA->>Broker: PUBLISH to path/set {"state":"ON"}
    Broker->>MQTT: Message received
    MQTT->>Light: onMessage callback
    Light->>Light: Parse: "ON" → ledOn = true
    Light->>Light: Update GPIO: LED turns ON
    Light->>MQTT: Publish state confirmation
    MQTT->>Broker: PUBLISH to path/state {"state":"ON"}
    Broker->>HA: State confirmation
    HA->>HA: UI shows light ON
    
    Note over Device,HA: User Controls Light from Device
    Device->>Light: WebSocket update: led_on = false
    Light->>Light: State updated
    Light->>Light: Update GPIO: LED turns OFF
    Light->>MQTT: Auto-publish via update handler
    MQTT->>Broker: PUBLISH to path/state {"state":"OFF"}
    Broker->>HA: State update
    HA->>HA: UI shows light OFF
```

## Factory Reset Flow

```mermaid
sequenceDiagram
    participant Browser as Web Browser
    participant Server as AsyncWebServer
    participant FactoryReset as FactoryResetService
    participant FS as LittleFS
    participant ESP as ESP Module
    participant Services as All Services
    
    Browser->>Server: POST /rest/factoryReset
    Server->>FactoryReset: factoryReset()
    
    FactoryReset->>FS: Open /config directory
    FS-->>FactoryReset: Directory handle
    
    loop For each file in /config
        FactoryReset->>FS: Delete file
        Note over FS: wifiSettings.json deleted<br/>mqttSettings.json deleted<br/>securitySettings.json deleted<br/>...
    end
    
    FactoryReset-->>Browser: 200 OK "Factory reset complete"
    
    FactoryReset->>ESP: ESP.restart()
    Note over ESP: Device reboots
    
    Note over ESP,Services: After Reboot
    ESP->>Services: All services begin()
    Services->>FS: readFromFS()
    
    alt Config File Missing
        FS-->>Services: File not found
        Services->>Services: applyDefaults()
        Note over Services: Factory defaults applied:<br/>- AP: ESP8266-React-{id}<br/>- Admin: admin/admin<br/>- MQTT: disabled
        Services->>FS: writeToFS()
        Note over FS: Create new config files<br/>with defaults
    end
    
    Services-->>ESP: Initialized with factory settings
```

## OTA Update Flow

```mermaid
sequenceDiagram
    participant Browser as Web Browser
    participant Server as AsyncWebServer
    participant Upload as UploadFirmwareService
    participant OTA as ArduinoOTA
    participant Flash as Flash Memory
    participant ESP as ESP Module
    
    Browser->>Browser: User selects firmware .bin file
    Browser->>Server: POST /rest/uploadFirmware<br/>multipart/form-data
    Server->>Upload: handleUpload(request, filename, data)
    
    Upload->>OTA: Begin OTA update
    OTA->>Flash: Erase update partition
    
    loop For each chunk
        Browser->>Server: Send firmware chunk
        Server->>Upload: Write chunk
        Upload->>OTA: Write to flash
        OTA->>Flash: Write bytes
        Upload-->>Browser: Progress update
    end
    
    Upload->>OTA: End OTA update
    OTA->>Flash: Finalize and verify
    
    alt Update Successful
        OTA-->>Upload: Success
        Upload-->>Browser: 200 OK "Update successful"
        
        Upload->>ESP: ESP.restart()
        Note over ESP: Device reboots with new firmware
        
        ESP->>ESP: Boot from new partition
        ESP->>ESP: Mark partition as valid
        
        Browser->>Browser: Poll /rest/systemStatus
        Note over Browser: Wait for device to come back online
        
        ESP-->>Browser: Device responds
        Browser->>Browser: Show "Update successful" message
    else Update Failed
        OTA-->>Upload: Error
        Upload-->>Browser: 500 Internal Server Error
        Note over Upload: Old firmware still active
    end
```

## Next Steps

- [DATA-FLOWS.md](DATA-FLOWS.md) - Data movement patterns
- [API-REFERENCE.md](API-REFERENCE.md) - Complete API documentation
- [DESIGN-PATTERNS.md](DESIGN-PATTERNS.md) - Implementation patterns
