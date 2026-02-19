#ifndef DEVICE_CONTEXT_H
#define DEVICE_CONTEXT_H

#include <Arduino.h>
#include <WiFi.h>
#include "../include/types/device_stats.h"

// Forward-declare subsystems — headers included only in .cpp
class WiFiManager;
class BLEManager;

/**
 * Central owner of all runtime state and subsystem pointers.
 *
 * Subsystems (WiFiManager, BLEManager) read/write DeviceStats through
 * this singleton rather than through scattered globals.
 */
class DeviceContext {
public:
    static DeviceContext& getInstance();

    void setup();
    void loop();

    // ── State ────────────────────────────────────────────────────────
    DeviceStats&  getStats();
    TransportMode getTransport() const;
    void          setTransport(TransportMode mode);

    // ── Subsystem access ─────────────────────────────────────────────
    WiFiManager* getWiFiManager();
    BLEManager*  getBLEManager();

    // ── Lifecycle events (called by subsystem callbacks) ─────────────
    void onWiFiConnected();
    void onWiFiDisconnected();
    void onBLEConnected();
    void onBLEDisconnected();

    // ── Stats broadcast ──────────────────────────────────────────────
    void requestStatusBroadcast();
    String buildStatusJson() const;

private:
    DeviceContext();
    DeviceContext(const DeviceContext&)            = delete;
    DeviceContext& operator=(const DeviceContext&) = delete;

    DeviceStats  stats;
    WiFiManager* wifiMgr;
    BLEManager*  bleMgr;

    bool statusBroadcastRequested;

    // ── Helpers ──────────────────────────────────────────────────────
    void refreshDeviceStats();
    void broadcastStats();

    // Motor
    static constexpr int MOTOR_PWM_PIN = 4;
    static constexpr int LED_PIN       = 2;
};

#endif // DEVICE_CONTEXT_H
