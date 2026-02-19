#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

/**
 * Singleton that centralises all NVS (non-volatile storage) access.
 * Replaces the old WiFiCredentials helper and any other scattered
 * Preferences usage so every key lives under one namespace.
 */
class ConfigManager {
public:
    static ConfigManager& getInstance();

    // WiFi
    void   setWiFiCredentials(const String& ssid, const String& password);
    String getWiFiSSID();
    String getWiFiPassword();
    bool   hasWiFiCredentials();
    void   clearWiFiCredentials();

    // Device
    void   setDeviceName(const String& name);
    String getDeviceName();

    // Transport
    void setLastTransport(int transport);
    int  getLastTransport();

    // Remote server
    void   setRemoteServer(const String& url);
    String getRemoteServer();

private:
    ConfigManager() = default;
    ConfigManager(const ConfigManager&)            = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    static constexpr const char* NS = "openvibe";
};

#endif // CONFIG_MANAGER_H
