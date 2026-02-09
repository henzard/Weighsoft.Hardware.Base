#include <ESP8266React.h>
#include <LightMqttSettingsService.h>
#include <LightStateService.h>

#define SERIAL_BAUD_RATE 115200

// Use pointers to avoid early construction issues on ESP32
AsyncWebServer* server;
ESP8266React* esp8266React;
LightMqttSettingsService* lightMqttSettingsService;
LightStateService* lightStateService;

void setup() {
  // start serial and filesystem
  Serial.begin(SERIAL_BAUD_RATE);
  delay(500);
  
  Serial.println(F("\n\n=== Weighsoft Hardware UI Starting ==="));
  #ifdef ESP32
  Serial.print(F("ESP-IDF version: "));
  Serial.println(esp_get_idf_version());
  #endif
  Serial.print(F("Free heap: "));
  Serial.println(ESP.getFreeHeap());
  
  Serial.println(F("[1/6] Creating web server..."));
  server = new AsyncWebServer(80);
  Serial.println(F("[1/6] Web server created OK"));
  
  Serial.println(F("[2/6] Initializing framework..."));
  esp8266React = new ESP8266React(server);
  Serial.println(F("[2/6] Framework created OK"));
  
  Serial.println(F("[3/6] Starting framework services..."));
  esp8266React->begin();
  Serial.println(F("[3/6] Framework initialized OK"));

  Serial.println(F("[4/6] Initializing light MQTT service..."));
  lightMqttSettingsService = new LightMqttSettingsService(
      server, esp8266React->getFS(), esp8266React->getSecurityManager());
  Serial.println(F("[4/6] Light MQTT service created OK"));
  
  Serial.println(F("[5/6] Initializing light state service..."));
  lightStateService = new LightStateService(
      server,
      esp8266React->getSecurityManager(),
      esp8266React->getMqttClient(),
      lightMqttSettingsService);
  Serial.println(F("[5/6] Light state service created OK"));

  // load the initial light settings
  lightStateService->begin();
  Serial.println(F("[5/6] Light settings loaded OK"));

  // start the light service
  lightMqttSettingsService->begin();
  Serial.println(F("[5/6] MQTT settings loaded OK"));

  Serial.println(F("[6/6] Starting web server..."));
  // start the server
  server->begin();
  Serial.println(F("[6/6] Web server started OK"));
  
  Serial.println(F("=== System Ready! ==="));
  Serial.print(F("Free heap after init: "));
  Serial.println(ESP.getFreeHeap());
}

void loop() {
  // run the framework's loop function
  esp8266React->loop();
}
