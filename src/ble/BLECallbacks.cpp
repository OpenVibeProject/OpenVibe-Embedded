#include "BLECallbacks.h"
#include "../DeviceContext.h"
#include "../ConfigManager.h"
#include "../wifi/WiFiManager.h"
#include <ArduinoJson.h>

// ── Server connect / disconnect ──────────────────────────────────────

void BLEServerHandler::onConnect(BLEServer* server) {
    DeviceContext::getInstance().onBLEConnected();
}

void BLEServerHandler::onDisconnect(BLEServer* server) {
    DeviceContext::getInstance().onBLEDisconnected();
    BLEDevice::startAdvertising();   // resume advertising
}

// ── Write handler for the WiFi / command characteristic ──────────────

void WiFiConfigCharacteristicHandler::onWrite(BLECharacteristic* characteristic) {
    std::string raw = characteristic->getValue();
    if (raw.empty()) return;

    Serial.printf("[BLE] Received: %s\n", raw.c_str());

    JsonDocument doc;
    if (deserializeJson(doc, raw.c_str())) {
        Serial.println("[BLE] JSON parse failed");
        return;
    }

    DeviceContext& ctx = DeviceContext::getInstance();
    ConfigManager& cfg = ConfigManager::getInstance();

    const char* req = doc["requestType"];
    if (!req) return;

    // ── STATUS ───────────────────────────────────────────────────────
    if (strcmp(req, "STATUS") == 0) {
        ctx.requestStatusBroadcast();
    }
    // ── INTENSITY ────────────────────────────────────────────────────
    else if (strcmp(req, "INTENSITY") == 0) {
        int val = doc["intensity"].as<int>();
        ctx.getStats().intensity = constrain(val, 0, 100);
        Serial.printf("[BLE] Intensity → %d\n", ctx.getStats().intensity);
    }
    // ── WIFI_CREDENTIALS (non-blocking!) ─────────────────────────────
    else if (strcmp(req, "WIFI_CREDENTIALS") == 0) {
        const char* ssid = doc["ssid"];
        const char* pass = doc["password"];
        if (!ssid) return;

        Serial.printf("[BLE] Saving WiFi creds for \"%s\"\n", ssid);
        cfg.setWiFiCredentials(ssid, pass ? pass : "");

        WiFiManager* wifi = ctx.getWiFiManager();
        if (wifi) wifi->connect();   // returns immediately
    }
    // ── SWITCH_TRANSPORT ─────────────────────────────────────────────
    else if (strcmp(req, "SWITCH_TRANSPORT") == 0) {
        const char* t = doc["transport"];
        if (!t) return;

        TransportMode mode = ctx.getTransport();
        if      (strcmp(t, "BLE")    == 0) mode = TRANSPORT_BLE;
        else if (strcmp(t, "WIFI")   == 0) mode = TRANSPORT_WIFI;
        else if (strcmp(t, "REMOTE") == 0) {
            mode = TRANSPORT_REMOTE;
            const char* addr = doc["serverAddress"];
            if (addr) {
                cfg.setRemoteServer(addr);
                ctx.getStats().serverAddress = String(addr);
            }
        }

        ctx.setTransport(mode);
        Serial.printf("[BLE] Transport → %s\n", t);
    }
}
