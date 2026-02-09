#include <ESP8266React.h>
#include <examples/led/LedExampleService.h>
#include <examples/serial/SerialService.h>

#define SERIAL_BAUD_RATE 115200

// Use pointers to avoid early construction issues on ESP32
AsyncWebServer* server;
ESP8266React* esp8266React;
LedExampleService* ledExampleService;
SerialService* serialService;

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

  Serial.println(F("[4/6] Initializing LED example service..."));
  ledExampleService = new LedExampleService(
      server,
      esp8266React->getSecurityManager(),
      esp8266React->getMqttClient()
#if FT_ENABLED(FT_BLE)
      ,nullptr  // BLE server will be configured via callback
#endif
      );
  Serial.println(F("[4/6] LED example service created OK"));

#if FT_ENABLED(FT_BLE)
  // Register callbacks to configure BLE when server is ready
  esp8266React->getBleSettingsService()->onBleServerStarted(
    [](BLEServer* bleServer) {
      // LED callback
      if (ledExampleService) {
        Serial.println(F("[LED] BLE server ready callback received"));
        ledExampleService->setBleServer(bleServer);
        ledExampleService->configureBle();
      }
      // Serial callback
      if (serialService) {
        Serial.println(F("[Serial] BLE server ready callback received"));
        serialService->setBleServer(bleServer);
        serialService->configureBle();
      }
    }
  );
  Serial.println(F("[4/5] BLE callbacks registered OK"));
#endif

  // load the initial LED settings
  ledExampleService->begin();
  Serial.println(F("[4/7] LED example loaded OK"));

  Serial.println(F("[5/7] Initializing Serial monitor service..."));
  serialService = new SerialService(
      server,
      esp8266React->getSecurityManager(),
      esp8266React->getMqttClient()
#if FT_ENABLED(FT_BLE)
      ,nullptr  // BLE server will be configured via callback
#endif
      );
  serialService->begin();
  Serial.println(F("[5/7] Serial service loaded OK"));

  Serial.println(F("[6/7] Starting web server..."));
  // start the server
  server->begin();
  Serial.println(F("[6/7] Web server started OK"));
  
  Serial.println(F("=== System Ready! ==="));
  Serial.print(F("Free heap after init: "));
  Serial.println(ESP.getFreeHeap());
}

void loop() {
  // run the framework's loop function
  esp8266React->loop();
  
  // read serial data
  serialService->loop();
}
