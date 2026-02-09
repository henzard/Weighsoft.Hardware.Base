#include <BleSettingsService.h>

#include <Features.h>

#if FT_ENABLED(FT_BLE)

BleSettingsService::BleSettingsService(AsyncWebServer* server,
                                       FS* fs,
                                       SecurityManager* securityManager) :
    _httpEndpoint(BleSettings::read,
                  BleSettings::update,
                  this,
                  server,
                  BLE_SETTINGS_PATH,
                  securityManager,
                  AuthenticationPredicates::IS_AUTHENTICATED),
    _fsPersistence(BleSettings::read, BleSettings::update, this, fs, BLE_SETTINGS_FILE),
    _bleServer(nullptr) {
  addUpdateHandler([&](const String& originId) { onConfigUpdated(); }, false);
}

void BleSettingsService::begin() {
  _fsPersistence.readFromFS();
  onConfigUpdated();
}

void BleSettingsService::onConfigUpdated() {
  if (_state.enabled) {
    startBleServer();
  } else {
    stopBleServer();
  }
}

void BleSettingsService::startBleServer() {
  if (_bleServer != nullptr) {
    return;  // Already running
  }

  Serial.println("[BLE] Starting BLE server...");
  BLEDevice::init(_state.deviceName.c_str());
  _bleServer = BLEDevice::createServer();

  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLEUUID((uint16_t)0x1800));  // Generic Access
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.printf("[BLE] BLE server started: %s\n", _state.deviceName.c_str());
}

void BleSettingsService::stopBleServer() {
  if (_bleServer == nullptr) {
    return;  // Already stopped
  }

  Serial.println("[BLE] Stopping BLE server...");
  BLEDevice::deinit(false);
  _bleServer = nullptr;
  Serial.println("[BLE] BLE server stopped");
}

#endif  // FT_ENABLED(FT_BLE)
