#ifndef BLE_CALLBACKS_H
#define BLE_CALLBACKS_H

#include <BLEServer.h>
#include <BLECharacteristic.h>

class BLEServerHandler : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override;
  void onDisconnect(BLEServer* server) override;
};

class WiFiConfigCharacteristicHandler : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override;
};

#endif // BLE_CALLBACKS_H
