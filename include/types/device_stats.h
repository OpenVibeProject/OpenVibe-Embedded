#ifndef DEVICE_STATS_H
#define DEVICE_STATS_H

#include <Arduino.h>

enum TransportMode {
    TRANSPORT_BLE = 0,
    TRANSPORT_WIFI = 1,
    TRANSPORT_REMOTE = 2
};

/**
 * Holds the runtime state of the device.
 * Owned exclusively by DeviceContext — never accessed via extern.
 */
struct DeviceStats {
    int intensity = 0;
    int battery = 100;
    bool isCharging = false;
    bool isBluetoothConnected = false;
    bool isWifiConnected = false;
    String ipAddress;
    String macAddress;
    const char* version = "1.0.0";
    TransportMode transport = TRANSPORT_BLE;
    String serverAddress;
};

#endif // DEVICE_STATS_H
