#ifndef DEVICE_STATS_H
#define DEVICE_STATS_H

#include <Arduino.h>
#include <WiFi.h>

enum TransportMode {
    TRANSPORT_BLE,
    TRANSPORT_WIFI,
    TRANSPORT_REMOTE
};

// Device state structure
struct DeviceStats {
    int intensity = 0;
    int battery = 0;
    bool isCharging = false;
    bool isBluetoothConnected = false;
    bool isWifiConnected = false;
    String ipAddress;
    String macAddress;
    const char* version = "1.0.0";
    TransportMode transport = TRANSPORT_BLE;
    String serverAddress;
};

// Shared connection flag (defined in a single translation unit)
extern volatile bool deviceConnected;

#endif // DEVICE_STATS_H
