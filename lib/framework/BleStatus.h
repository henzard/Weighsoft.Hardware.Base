#ifndef BleStatus_h
#define BleStatus_h

#include <Features.h>

#if FT_ENABLED(FT_BLE)

#include <HttpEndpoint.h>
#include <BLEServer.h>

#define BLE_STATUS_PATH "/rest/bleStatus"

class BleStatusData {
 public:
  bool enabled;
  uint32_t connectedDevices;
  String deviceName;
  String macAddress;

  static void read(BleStatusData& status, JsonObject& root) {
    root["enabled"] = status.enabled;
    root["connected_devices"] = status.connectedDevices;
    root["device_name"] = status.deviceName;
    root["mac_address"] = status.macAddress;
  }

  static StateUpdateResult update(JsonObject& root, BleStatusData& status) {
    // Read-only, no updates
    return StateUpdateResult::UNCHANGED;
  }
};

class BleStatus : public StatefulService<BleStatusData> {
 public:
  BleStatus(AsyncWebServer* server, SecurityManager* securityManager, BLEServer* bleServer);
  void updateStatus();

 private:
  HttpEndpoint<BleStatusData> _httpEndpoint;
  BLEServer* _bleServer;
};

#endif  // FT_ENABLED(FT_BLE)
#endif  // BleStatus_h
