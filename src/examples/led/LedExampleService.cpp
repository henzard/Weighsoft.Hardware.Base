#include <examples/led/LedExampleService.h>

LedExampleService::LedExampleService(AsyncWebServer* server,
                                     SecurityManager* securityManager,
                                     AsyncMqttClient* mqttClient) :
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
    _mqttClient(mqttClient) {
  
  // Inline MQTT configuration using SettingValue placeholders
  // Single-layer pattern - no separate settings service needed
  _mqttBasePath = SettingValue::format("homeassistant/light/#{unique_id}");
  _mqttName = SettingValue::format("led-example-#{unique_id}");
  _mqttUniqueId = SettingValue::format("led-#{unique_id}");
  
  // configure led to be output
  pinMode(LED_PIN, OUTPUT);

  // configure MQTT callback
  _mqttClient->onConnect(std::bind(&LedExampleService::configureMqtt, this));

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
