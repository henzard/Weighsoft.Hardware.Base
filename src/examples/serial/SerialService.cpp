#include <examples/serial/SerialService.h>

SerialService::SerialService(AsyncWebServer* server,
                             SecurityManager* securityManager,
                             AsyncMqttClient* mqttClient
#if FT_ENABLED(FT_BLE)
                             ,BLEServer* bleServer
#endif
                             ) :
    _httpEndpoint(SerialState::read,
                  SerialState::update,
                  this,
                  server,
                  SERIAL_ENDPOINT_PATH,
                  securityManager,
                  AuthenticationPredicates::IS_AUTHENTICATED),
    _mqttPubSub(SerialState::read, SerialState::update, this, mqttClient),
    _webSocket(SerialState::read,
               SerialState::update,
               this,
               server,
               SERIAL_SOCKET_PATH,
               securityManager,
               AuthenticationPredicates::IS_AUTHENTICATED),
    _mqttClient(mqttClient)
#if FT_ENABLED(FT_BLE)
    ,_blePubSub(SerialState::read, SerialState::update, this, bleServer),
    _bleServer(bleServer),
    _bleService(nullptr),
    _bleCharacteristic(nullptr)
#endif
{
  // Inline MQTT configuration using SettingValue placeholders
  // Single-layer pattern - no separate settings service needed
  _mqttBasePath = SettingValue::format("weighsoft/serial/#{unique_id}");
  _mqttName = SettingValue::format("serial-monitor-#{unique_id}");
  _mqttUniqueId = SettingValue::format("serial-#{unique_id}");
  
  // Configure MQTT callback
  _mqttClient->onConnect(std::bind(&SerialService::configureMqtt, this));
  
  // No update handler needed - data flows one way (hardware → channels)
}

void SerialService::begin() {
  // Initialize Serial2 (ESP32 GPIO16=RX, GPIO17=TX)
  Serial2.begin(115200);
  _state.lastLine = "";
  _state.timestamp = 0;
  _lineBuffer = "";
  
  Serial.println(F("[Serial] Serial2 initialized at 115200 baud"));
  Serial.println(F("[Serial] RX=GPIO16, TX=GPIO17"));
}

void SerialService::loop() {
  readSerial();
}

void SerialService::readSerial() {
  while (Serial2.available()) {
    char c = Serial2.read();
    
    if (c == '\n') {
      // Complete line received
      if (_lineBuffer.length() > 0) {
        // Update state with new line - this will notify all channels
        update([&](SerialState& state) {
          state.lastLine = _lineBuffer;
          state.timestamp = millis();
          return StateUpdateResult::CHANGED;
        }, "serial_hw");  // Origin ID prevents loops (though none expected)
      }
      _lineBuffer = "";
    } else if (c != '\r') {  // Ignore carriage returns
      _lineBuffer += c;
      
      // Safety: limit line length to prevent memory issues
      if (_lineBuffer.length() > 512) {
        Serial.println(F("[Serial] WARNING: Line exceeded 512 chars, discarded"));
        _lineBuffer = "";  // Discard oversized line
      }
    }
  }
}

void SerialService::configureMqtt() {
  if (!_mqttClient->connected()) {
    return;
  }
  
  // Build topics from inline configuration
  String pubTopic = _mqttBasePath + "/data";
  String subTopic = "";  // Read-only, no subscription needed
  
  // Configure MqttPubSub topics (empty subTopic = publish only)
  _mqttPubSub.configureTopics(pubTopic, subTopic);
  
  Serial.printf("[Serial] MQTT configured - topic: %s\n", pubTopic.c_str());
}

#if FT_ENABLED(FT_BLE)
void SerialService::configureBle() {
  if (_bleServer == nullptr) {
    Serial.println("[Serial] BLE server not available, skipping BLE configuration");
    return;
  }
  
  Serial.println("[Serial] Configuring BLE service...");
  
  // Create BLE service with inline UUID
  _bleService = _bleServer->createService(BLE_SERVICE_UUID);
  
  // Create BLE characteristic with inline UUID (read-only with notifications)
  _bleCharacteristic = _bleService->createCharacteristic(
      BLE_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_NOTIFY  // No WRITE - read-only
  );
  
  // Configure BlePubSub to use this characteristic
  _blePubSub.configureCharacteristic(_bleCharacteristic);
  
  // Start the service
  _bleService->start();
  
  Serial.printf("[Serial] BLE service configured - Service UUID: %s, Char UUID: %s\n", 
                BLE_SERVICE_UUID, BLE_CHAR_UUID);
}
#endif
