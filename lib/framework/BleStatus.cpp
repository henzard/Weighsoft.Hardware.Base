#include <BleStatus.h>

#include <Features.h>

#if FT_ENABLED(FT_BLE)

#include <BLEDevice.h>

BleStatus::BleStatus(AsyncWebServer* server, SecurityManager* securityManager, BLEServer* bleServer) :
    _httpEndpoint(BleStatusData::read,
                  BleStatusData::update,
                  this,
                  server,
                  BLE_STATUS_PATH,
                  securityManager),
    _bleServer(bleServer) {
}

void BleStatus::updateStatus() {
  update([&](BleStatusData& status) {
    status.enabled = (_bleServer != nullptr);
    status.connectedDevices = (_bleServer != nullptr) ? _bleServer->getConnectedCount() : 0;
    
    // Only access BLEDevice if server exists
    if (_bleServer != nullptr) {
      std::string bleDeviceName = BLEDevice::toString();
      status.deviceName = String(bleDeviceName.c_str());
      
      // Get BLE MAC address
      uint8_t mac[6];
      esp_read_mac(mac, ESP_MAC_BT);
      char macStr[18];
      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      status.macAddress = String(macStr);
    } else {
      status.deviceName = "";
      status.macAddress = "";
    }
    
    return StateUpdateResult::CHANGED;
  }, "system");
}

#endif  // FT_ENABLED(FT_BLE)
