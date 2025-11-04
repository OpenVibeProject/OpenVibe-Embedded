#ifndef DEVICE_STATS_H
#define DEVICE_STATS_H

#include <Arduino.h>
#include <WiFi.h>

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
};

// Shared connection flag (defined in a single translation unit)
extern volatile bool deviceConnected;

#endif // DEVICE_STATS_H
