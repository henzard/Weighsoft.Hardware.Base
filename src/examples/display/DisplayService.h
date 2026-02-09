#ifndef DisplayService_h
#define DisplayService_h

#include <HttpEndpoint.h>
#include <MqttPubSub.h>
#include <WebSocketTxRx.h>
#include <SettingValue.h>
#include <LiquidCrystal_I2C.h>

#if FT_ENABLED(FT_BLE)
#include <BlePubSub.h>
#include <BLEServer.h>
#include <BLEService.h>
#include <BLECharacteristic.h>
#endif

#define DISPLAY_ENDPOINT_PATH "/rest/display"
#define DISPLAY_SOCKET_PATH "/ws/display"

class DisplayState {
 public:
  String line1;
  String line2;
  uint8_t i2cAddress;
  bool backlight;

  static void read(DisplayState& state, JsonObject& root) {
    root["line1"] = state.line1;
    root["line2"] = state.line2;
    root["i2c_address"] = state.i2cAddress;
    root["backlight"] = state.backlight;
  }

  static StateUpdateResult update(JsonObject& root, DisplayState& state) {
    bool changed = false;
    
    String newLine1 = root["line1"] | "";
    if (newLine1.length() > 16) newLine1 = newLine1.substring(0, 16);
    if (state.line1 != newLine1) {
      state.line1 = newLine1;
      changed = true;
    }
    
    String newLine2 = root["line2"] | "";
    if (newLine2.length() > 16) newLine2 = newLine2.substring(0, 16);
    if (state.line2 != newLine2) {
      state.line2 = newLine2;
      changed = true;
    }
    
    uint8_t newAddress = root["i2c_address"] | 0x27;
    if (state.i2cAddress != newAddress) {
      state.i2cAddress = newAddress;
      changed = true;
    }
    
    bool newBacklight = root["backlight"] | true;
    if (state.backlight != newBacklight) {
      state.backlight = newBacklight;
      changed = true;
    }
    
    return changed ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
  }
};

class DisplayService : public StatefulService<DisplayState> {
 public:
  DisplayService(AsyncWebServer* server,
                    SecurityManager* securityManager,
                    AsyncMqttClient* mqttClient
#if FT_ENABLED(FT_BLE)
                    ,BLEServer* bleServer
#endif
                    );
  void begin();

#if FT_ENABLED(FT_BLE)
  void setBleServer(BLEServer* bleServer) { _bleServer = bleServer; }
  void configureBle();
#endif

 private:
  HttpEndpoint<DisplayState> _httpEndpoint;
  MqttPubSub<DisplayState> _mqttPubSub;
  WebSocketTxRx<DisplayState> _webSocket;
  AsyncMqttClient* _mqttClient;

  // Hardware
  LiquidCrystal_I2C* _lcd;

  // Inline MQTT configuration - single-layer pattern
  String _mqttBasePath;
  String _mqttName;
  String _mqttUniqueId;

#if FT_ENABLED(FT_BLE)
  BlePubSub<DisplayState> _blePubSub;
  BLEServer* _bleServer;
  BLEService* _bleService;
  BLECharacteristic* _bleCharacteristic;
  
  // Inline BLE configuration - single-layer pattern
  static constexpr const char* BLE_SERVICE_UUID = "a8f3d5e0-8b2c-4f1a-9d6e-3c7b4a5f1e8d";
  static constexpr const char* BLE_CHAR_UUID = "a8f3d5e1-8b2c-4f1a-9d6e-3c7b4a5f1e8d";
#endif

  void configureMqtt();
  void onConfigUpdated();
};

#endif
