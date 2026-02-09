#ifndef BleSettingsService_h
#define BleSettingsService_h

#include <Features.h>

#if FT_ENABLED(FT_BLE)

#include <functional>
#include <HttpEndpoint.h>
#include <FSPersistence.h>
#include <SettingValue.h>
#include <BLEServer.h>
#include <BLEDevice.h>
#include <BLEUtils.h>

#define BLE_SETTINGS_FILE "/config/bleSettings.json"
#define BLE_SETTINGS_PATH "/rest/bleSettings"

class BleSettings {
 public:
  bool enabled;
  String deviceName;

  static void read(BleSettings& settings, JsonObject& root) {
    root["enabled"] = settings.enabled;
    root["device_name"] = settings.deviceName;
  }

  static StateUpdateResult update(JsonObject& root, BleSettings& settings) {
    bool newEnabled = root["enabled"] | false;
    String newDeviceName = root["device_name"] | SettingValue::format("Weighsoft-#{unique_id}");

    bool changed = false;
    if (settings.enabled != newEnabled) {
      settings.enabled = newEnabled;
      changed = true;
    }
    if (settings.deviceName != newDeviceName) {
      settings.deviceName = newDeviceName;
      changed = true;
    }

    return changed ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
  }
};

class BleSettingsService : public StatefulService<BleSettings> {
 public:
  BleSettingsService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager);
  void begin();

  BLEServer* getBleServer() { return _bleServer; }
  bool isEnabled() { return _state.enabled; }

  // Callback for when BLE server is started/stopped
  using BleServerCallback = std::function<void(BLEServer*)>;
  void onBleServerStarted(BleServerCallback callback) {
    _onServerStartedCallback = callback;
  }

 private:
  HttpEndpoint<BleSettings> _httpEndpoint;
  FSPersistence<BleSettings> _fsPersistence;
  BLEServer* _bleServer;
  BleServerCallback _onServerStartedCallback;

  void onConfigUpdated();
  void startBleServer();
  void stopBleServer();
};

#endif  // FT_ENABLED(FT_BLE)
#endif  // BleSettingsService_h
