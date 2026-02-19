#ifndef BLE_CALLBACKS_H
#define BLE_CALLBACKS_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>

/**
 * BLE callback handlers.
 *
 * These are instantiated by BLEManager and route events to
 * DeviceContext so that no global variables are required.
 */
class BLEServerHandler : public BLEServerCallbacks {
    void onConnect(BLEServer* server) override;
    void onDisconnect(BLEServer* server) override;
};

class WiFiConfigCharacteristicHandler : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override;
};

#endif // BLE_CALLBACKS_H
