#include <examples/display/DisplayService.h>

DisplayService::DisplayService(AsyncWebServer* server,
                                     SecurityManager* securityManager,
                                     AsyncMqttClient* mqttClient
#if FT_ENABLED(FT_BLE)
                                     ,BLEServer* bleServer
#endif
                                     ) :
    _httpEndpoint(DisplayState::read,
                  DisplayState::update,
                  this,
                  server,
                  DISPLAY_ENDPOINT_PATH,
                  securityManager,
                  AuthenticationPredicates::IS_AUTHENTICATED),
    _mqttPubSub(DisplayState::read, DisplayState::update, this, mqttClient),
    _webSocket(DisplayState::read,
               DisplayState::update,
               this,
               server,
               DISPLAY_SOCKET_PATH,
               securityManager,
               AuthenticationPredicates::IS_AUTHENTICATED),
    _mqttClient(mqttClient),
    _lcd(nullptr)
#if FT_ENABLED(FT_BLE)
    ,_blePubSub(DisplayState::read, DisplayState::update, this, bleServer),
    _bleServer(bleServer),
    _bleService(nullptr),
    _bleCharacteristic(nullptr)
#endif
{
  
  // Inline MQTT configuration using SettingValue placeholders
  // Single-layer pattern - no separate settings service needed
  _mqttBasePath = SettingValue::format("weighsoft/display/#{unique_id}");
  _mqttName = SettingValue::format("display-#{unique_id}");
  _mqttUniqueId = SettingValue::format("display-#{unique_id}");
  
  // configure MQTT callback
  _mqttClient->onConnect(std::bind(&DisplayService::configureMqtt, this));

  // Note: BLE will be configured via callback when BLE server is ready
  // This prevents initialization order issues

  // configure update handler to update LCD state for ALL channels
  // Origin tracking prevents feedback loops automatically
  addUpdateHandler([&](const String& originId) { onConfigUpdated(); }, false);
}

void DisplayService::begin() {
  // Initialize state with defaults
  _state.line1 = "Weighsoft";
  _state.line2 = "Display Ready";
  _state.i2cAddress = 0x27;
  _state.backlight = true;
  
  // Initialize LCD with current I2C address
  _lcd = new LiquidCrystal_I2C(_state.i2cAddress, 16, 2);
  _lcd->init();
  _lcd->backlight();
  
  // Display welcome message
  onConfigUpdated();
}

void DisplayService::onConfigUpdated() {
  if (_lcd == nullptr) {
    return;
  }
  
  DisplayState state = _state;
  
  // Handle I2C address change (requires reinit)
  static uint8_t lastAddress = 0x27;
  if (state.i2cAddress != lastAddress) {
    delete _lcd;
    _lcd = new LiquidCrystal_I2C(state.i2cAddress, 16, 2);
    _lcd->init();
    lastAddress = state.i2cAddress;
  }
  
  // Update display content
  _lcd->clear();
  _lcd->setCursor(0, 0);
  _lcd->print(state.line1);
  _lcd->setCursor(0, 1);
  _lcd->print(state.line2);
  
  // Control backlight
  if (state.backlight) {
    _lcd->backlight();
  } else {
    _lcd->noBacklight();
  }
}

void DisplayService::configureMqtt() {
  if (!_mqttClient->connected()) {
    return;
  }
  
  // Build topics from inline configuration
  String subTopic = _mqttBasePath + "/set";
  String pubTopic = _mqttBasePath + "/data";

  // Configure MqttPubSub topics
  _mqttPubSub.configureTopics(pubTopic, subTopic);
}

#if FT_ENABLED(FT_BLE)
void DisplayService::configureBle() {
  if (_bleServer == nullptr) {
    Serial.println("[Display] BLE server not available, skipping BLE configuration");
    return;
  }
  
  Serial.println("[Display] Configuring BLE service...");
  
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
  
  Serial.printf("[Display] BLE service configured - Service UUID: %s, Char UUID: %s\n", 
                BLE_SERVICE_UUID, BLE_CHAR_UUID);
}
#endif
