#ifndef SerialService_h
#define SerialService_h

#include <HttpEndpoint.h>
#include <MqttPubSub.h>
#include <WebSocketTxRx.h>
#include <SettingValue.h>
#include <examples/serial/SerialState.h>

#if FT_ENABLED(FT_BLE)
#include <BlePubSub.h>
#include <BLEServer.h>
#include <BLEService.h>
#include <BLECharacteristic.h>
#endif

#define SERIAL_ENDPOINT_PATH "/rest/serial"
#define SERIAL_SOCKET_PATH "/ws/serial"

class SerialService : public StatefulService<SerialState> {
 public:
  SerialService(AsyncWebServer* server,
                SecurityManager* securityManager,
                AsyncMqttClient* mqttClient
#if FT_ENABLED(FT_BLE)
                ,BLEServer* bleServer
#endif
                );
  void begin();
  void loop();  // Must be called in main loop() to read serial

#if FT_ENABLED(FT_BLE)
  void setBleServer(BLEServer* bleServer) { _bleServer = bleServer; }
  void configureBle();
#endif

 private:
  HttpEndpoint<SerialState> _httpEndpoint;
  MqttPubSub<SerialState> _mqttPubSub;
  WebSocketTxRx<SerialState> _webSocket;
  AsyncMqttClient* _mqttClient;
  
  // Inline MQTT configuration - single-layer pattern
  String _mqttBasePath;
  String _mqttName;
  String _mqttUniqueId;

#if FT_ENABLED(FT_BLE)
  BlePubSub<SerialState> _blePubSub;
  BLEServer* _bleServer;
  BLEService* _bleService;
  BLECharacteristic* _bleCharacteristic;
  
  // Inline BLE UUIDs - single-layer pattern
  static constexpr const char* BLE_SERVICE_UUID = "12340000-e8f2-537e-4f6c-d104768a1234";
  static constexpr const char* BLE_CHAR_UUID = "12340001-e8f2-537e-4f6c-d104768a1234";
#endif

  String _lineBuffer;  // Accumulates serial data until newline
  
  void configureMqtt();
  void readSerial();  // Called from loop()
};

#endif
