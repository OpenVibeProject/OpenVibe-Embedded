#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

/**
 * Encapsulates all BLE setup: server, service, characteristics,
 * advertising, and stats notification.
 *
 * Callbacks (BLEServerHandler / WiFiConfigCharacteristicHandler)
 * live in their own translation unit and talk to DeviceContext
 * directly — BLEManager just owns the ESP32 BLE objects.
 */
class BLEManager {
public:
    BLEManager();
    void begin(const String& deviceName);
    void updateStats(const String& jsonStats);
    bool isConnected() const;

private:
    BLEServer*         pServer;
    BLEService*        pService;
    BLECharacteristic* pWiFiChar;
    BLECharacteristic* pStatsChar;

    static constexpr const char* SERVICE_UUID   = "ec2e0883-782d-433b-9a0c-6d5df5565410";
    static constexpr const char* WIFI_CHAR_UUID = "c2433dd7-137e-4e82-845e-a40f70dc4a8d";
    static constexpr const char* STATS_CHAR_UUID = "c2433dd7-137e-4e82-845e-a40f70dc4a8e";
};

#endif // BLE_MANAGER_H
