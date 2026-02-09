#include <examples/led/LedExampleService.h>

LedExampleService::LedExampleService(AsyncWebServer* server,
                                     SecurityManager* securityManager,
                                     AsyncMqttClient* mqttClient
#if FT_ENABLED(FT_BLE)
                                     ,BLEServer* bleServer
#endif
                                     ) :
    _httpEndpoint(LedExampleState::read,
                  LedExampleState::update,
                  this,
                  server,
                  LED_EXAMPLE_ENDPOINT_PATH,
                  securityManager,
                  AuthenticationPredicates::IS_AUTHENTICATED),
    _mqttPubSub(LedExampleState::haRead, LedExampleState::haUpdate, this, mqttClient),
    _webSocket(LedExampleState::read,
               LedExampleState::update,
               this,
               server,
               LED_EXAMPLE_SOCKET_PATH,
               securityManager,
               AuthenticationPredicates::IS_AUTHENTICATED),
    _mqttClient(mqttClient)
#if FT_ENABLED(FT_BLE)
    ,_blePubSub(LedExampleState::read, LedExampleState::update, this, bleServer),
    _bleServer(bleServer),
    _bleService(nullptr),
    _bleCharacteristic(nullptr)
#endif
{
  
  // Inline MQTT configuration using SettingValue placeholders
  // Single-layer pattern - no separate settings service needed
  _mqttBasePath = SettingValue::format("homeassistant/light/#{unique_id}");
  _mqttName = SettingValue::format("led-example-#{unique_id}");
  _mqttUniqueId = SettingValue::format("led-#{unique_id}");
  
  // configure led to be output
  pinMode(LED_PIN, OUTPUT);

  // configure MQTT callback
  _mqttClient->onConnect(std::bind(&LedExampleService::configureMqtt, this));

#if FT_ENABLED(FT_BLE)
  // configure BLE service and characteristic
  if (_bleServer != nullptr) {
    configureBle();
  }
#endif

  // configure update handler to update LED state for ALL channels
  // Origin tracking prevents feedback loops automatically
  addUpdateHandler([&](const String& originId) { onConfigUpdated(); }, false);
}

void LedExampleService::begin() {
  _state.ledOn = DEFAULT_LED_STATE;
  onConfigUpdated();
}

void LedExampleService::onConfigUpdated() {
  digitalWrite(LED_PIN, _state.ledOn ? LED_ON : LED_OFF);
}

void LedExampleService::configureMqtt() {
  if (!_mqttClient->connected()) {
    return;
  }
  
  // Build topics from inline configuration
  String configTopic = _mqttBasePath + "/config";
  String subTopic = _mqttBasePath + "/set";
  String pubTopic = _mqttBasePath + "/state";

  // Configure MqttPubSub topics
  _mqttPubSub.configureTopics(pubTopic, subTopic);
  
  // Home Assistant auto-discovery
  DynamicJsonDocument doc(256);
  doc["~"] = _mqttBasePath;
  doc["name"] = _mqttName;
  doc["unique_id"] = _mqttUniqueId;
  doc["cmd_t"] = "~/set";
  doc["stat_t"] = "~/state";
  doc["schema"] = "json";
  doc["brightness"] = false;

  String payload;
  serializeJson(doc, payload);
  _mqttClient->publish(configTopic.c_str(), 0, false, payload.c_str());
}

#if FT_ENABLED(FT_BLE)
void LedExampleService::configureBle() {
  Serial.println("[LED] Configuring BLE service...");
  
  // Create BLE service with inline UUID
  _bleService = _bleServer->createService(BLE_SERVICE_UUID);
  
  // Create BLE characteristic with inline UUID
  _bleCharacteristic = _bleService->createCharacteristic(
      BLE_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_NOTIFY
  );
  
  // Configure BlePubSub to use this characteristic
  _blePubSub.configureCharacteristic(_bleCharacteristic);
  
  // Start the service
  _bleService->start();
  
  Serial.printf("[LED] BLE service configured - Service UUID: %s, Char UUID: %s\n", 
                BLE_SERVICE_UUID, BLE_CHAR_UUID);
}
#endif
