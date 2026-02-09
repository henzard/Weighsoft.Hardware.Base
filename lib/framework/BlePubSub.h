#ifndef BlePubSub_h
#define BlePubSub_h

#include <Features.h>

#if FT_ENABLED(FT_BLE)

#include <StatefulService.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <BLE2902.h>

#define BLE_ORIGIN_ID "ble"

template <class T>
class BleConnector {
 protected:
  StatefulService<T>* _statefulService;
  BLEServer* _bleServer;
  size_t _bufferSize;

  BleConnector(StatefulService<T>* statefulService, BLEServer* bleServer, size_t bufferSize) :
      _statefulService(statefulService), _bleServer(bleServer), _bufferSize(bufferSize) {
  }

 public:
  inline BLEServer* getBleServer() const {
    return _bleServer;
  }
};

template <class T>
class BlePub : virtual public BleConnector<T> {
 public:
  BlePub(JsonStateReader<T> stateReader,
         StatefulService<T>* statefulService,
         BLEServer* bleServer,
         BLECharacteristic* characteristic = nullptr,
         size_t bufferSize = DEFAULT_BUFFER_SIZE) :
      BleConnector<T>(statefulService, bleServer, bufferSize),
      _stateReader(stateReader),
      _characteristic(characteristic) {
    BleConnector<T>::_statefulService->addUpdateHandler(
        [&](const String& originId) { 
          if (originId != BLE_ORIGIN_ID) {
            notify(); 
          }
        }, 
        false);
  }

  void setCharacteristic(BLECharacteristic* characteristic) {
    _characteristic = characteristic;
    if (_characteristic) {
      _characteristic->addDescriptor(new BLE2902());
    }
  }

 protected:
  void notify() {
    if (_characteristic && BleConnector<T>::_bleServer->getConnectedCount() > 0) {
      // Serialize to JSON doc
      DynamicJsonDocument json(BleConnector<T>::_bufferSize);
      JsonObject jsonObject = json.to<JsonObject>();
      BleConnector<T>::_statefulService->read(jsonObject, _stateReader);

      // Serialize to string
      String payload;
      serializeJson(json, payload);

      // Notify BLE clients
      _characteristic->setValue(payload.c_str());
      _characteristic->notify();
    }
  }

 private:
  JsonStateReader<T> _stateReader;
  BLECharacteristic* _characteristic;
};

template <class T>
class BleSub : virtual public BleConnector<T> {
 public:
  BleSub(JsonStateUpdater<T> stateUpdater,
         StatefulService<T>* statefulService,
         BLEServer* bleServer,
         BLECharacteristic* characteristic = nullptr,
         size_t bufferSize = DEFAULT_BUFFER_SIZE) :
      BleConnector<T>(statefulService, bleServer, bufferSize),
      _stateUpdater(stateUpdater),
      _characteristic(characteristic) {
  }

  void setCharacteristic(BLECharacteristic* characteristic) {
    _characteristic = characteristic;
    if (_characteristic) {
      _characteristic->setCallbacks(new BleCallbacks(this));
    }
  }

 protected:
  void onBleWrite(const String& value) {
    // Parse JSON
    DynamicJsonDocument json(BleConnector<T>::_bufferSize);
    DeserializationError error = deserializeJson(json, value);

    if (!error && json.is<JsonObject>()) {
      JsonObject jsonObject = json.as<JsonObject>();
      BleConnector<T>::_statefulService->update(jsonObject, _stateUpdater, BLE_ORIGIN_ID);
    }
  }

 private:
  JsonStateUpdater<T> _stateUpdater;
  BLECharacteristic* _characteristic;

  class BleCallbacks : public BLECharacteristicCallbacks {
   public:
    BleCallbacks(BleSub* parent) : _parent(parent) {}

    void onWrite(BLECharacteristic* pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        _parent->onBleWrite(String(value.c_str()));
      }
    }

   private:
    BleSub* _parent;
  };
};

template <class T>
class BlePubSub : public BlePub<T>, public BleSub<T> {
 public:
  BlePubSub(JsonStateReader<T> stateReader,
            JsonStateUpdater<T> stateUpdater,
            StatefulService<T>* statefulService,
            BLEServer* bleServer,
            BLECharacteristic* characteristic = nullptr,
            size_t bufferSize = DEFAULT_BUFFER_SIZE) :
      BleConnector<T>(statefulService, bleServer, bufferSize),
      BlePub<T>(stateReader, statefulService, bleServer, characteristic, bufferSize),
      BleSub<T>(stateUpdater, statefulService, bleServer, characteristic, bufferSize) {
  }

  void configureCharacteristic(BLECharacteristic* characteristic) {
    BlePub<T>::setCharacteristic(characteristic);
    BleSub<T>::setCharacteristic(characteristic);
  }
};

#endif  // FT_ENABLED(FT_BLE)
#endif  // BlePubSub_h
