#include <ESP8266React.h>
#include <examples/led/LedExampleService.h>
#include <examples/serial/SerialService.h>
#include <examples/diagnostics/DiagnosticsService.h>
#include "VersionService.h"
#include "version.h"

#define SERIAL_BAUD_RATE 115200

// Use pointers to avoid early construction issues on ESP32
AsyncWebServer* server;
ESP8266React* esp8266React;
LedExampleService* ledExampleService;
SerialService* serialService;
DiagnosticsService* diagnosticsService;
VersionService* versionService;

void setup() {
  // start serial and filesystem
  Serial.begin(SERIAL_BAUD_RATE);
  delay(500);
  
  Serial.println(F("\n\n=== Weighsoft Hardware Base ==="));
  Serial.printf("Version: %s\n", VERSION_STRING);
  Serial.printf("Build: %s %s\n", BUILD_DATE, BUILD_TIME);
  Serial.printf("API: %s\n", API_VERSION);
  #ifdef ESP32
  Serial.print(F("ESP-IDF: "));
  Serial.println(esp_get_idf_version());
  #endif
  Serial.print(F("Free heap: "));
  Serial.println(ESP.getFreeHeap());
  Serial.println();
  
  Serial.println(F("[1/9] Creating web server..."));
  server = new AsyncWebServer(80);
  Serial.println(F("[1/9] Web server created OK"));
  
  Serial.println(F("[2/9] Initializing framework..."));
  esp8266React = new ESP8266React(server);
  Serial.println(F("[2/9] Framework created OK"));
  
  Serial.println(F("[3/9] Starting framework services..."));
  esp8266React->begin();
  Serial.println(F("[3/9] Framework initialized OK"));

  Serial.println(F("[4/9] Initializing version service..."));
  versionService = new VersionService(
      server,
      esp8266React->getSecurityManager()
      );
  versionService->begin();
  Serial.println(F("[4/9] Version service loaded OK"));

  Serial.println(F("[5/9] Initializing LED example service..."));
  ledExampleService = new LedExampleService(
      server,
      esp8266React->getSecurityManager(),
      esp8266React->getMqttClient()
#if FT_ENABLED(FT_BLE)
      ,nullptr  // BLE server will be configured via callback
#endif
      );
  Serial.println(F("[5/9] LED example service created OK"));

  // load the initial LED settings
  ledExampleService->begin();
  Serial.println(F("[5/9] LED example loaded OK"));

  Serial.println(F("[6/9] Initializing Serial monitor service..."));
  serialService = new SerialService(
      server,
      esp8266React->getFS(),
      esp8266React->getSecurityManager(),
      esp8266React->getMqttClient()
#if FT_ENABLED(FT_BLE)
      ,nullptr  // BLE server will be configured via callback
#endif
      );
  serialService->begin();
  Serial.println(F("[6/9] Serial service loaded OK"));

  Serial.println(F("[7/9] Initializing UART Diagnostics service..."));
  diagnosticsService = new DiagnosticsService(
      server,
      esp8266React->getSecurityManager()
      );
  diagnosticsService->begin();
  Serial.println(F("[7/9] Diagnostics service loaded OK"));

#if FT_ENABLED(FT_BLE)
  // Register callbacks after both services exist so callback never sees null
  esp8266React->getBleSettingsService()->onBleServerStarted(
    [](BLEServer* bleServer) {
      if (ledExampleService) {
        Serial.println(F("[LED] BLE server ready callback received"));
        ledExampleService->setBleServer(bleServer);
        ledExampleService->configureBle();
      }
      if (serialService) {
        Serial.println(F("[Serial] BLE server ready callback received"));
        serialService->setBleServer(bleServer);
        serialService->configureBle();
      }
    }
  );
  Serial.println(F("[8/9] BLE callbacks registered OK"));
#endif

  Serial.println(F("[9/9] Starting web server..."));
  // start the server
  server->begin();
  Serial.println(F("[9/9] Web server started OK"));
  
  Serial.println(F("=== System Ready! ==="));
  Serial.print(F("Free heap after init: "));
  Serial.println(ESP.getFreeHeap());
}

void loop() {
  // run the framework's loop function
  esp8266React->loop();
  
  // read serial data
  serialService->loop();
  
  // run diagnostic tests
  diagnosticsService->loop();
}
