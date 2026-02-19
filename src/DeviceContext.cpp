#include "DeviceContext.h"
#include "ConfigManager.h"
#include "wifi/WiFiManager.h"
#include "ble/BLEManager.h"
#include <ArduinoJson.h>
#include <base64.h>

// ── Singleton ────────────────────────────────────────────────────────

DeviceContext& DeviceContext::getInstance() {
    static DeviceContext inst;
    return inst;
}

DeviceContext::DeviceContext()
    : wifiMgr(nullptr)
    , bleMgr(nullptr)
    , statusBroadcastRequested(false) {}

// ── Lifecycle ────────────────────────────────────────────────────────

void DeviceContext::setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    ConfigManager& cfg = ConfigManager::getInstance();

    // ── BLE ──────────────────────────────────────────────────────────
    WiFi.mode(WIFI_STA);
    String deviceId = base64::encode(WiFi.macAddress());
    String fullName = cfg.getDeviceName() + "-" + deviceId.substring(0, 8);

    bleMgr = new BLEManager();
    bleMgr->begin(fullName);

    // ── WiFi ─────────────────────────────────────────────────────────
    wifiMgr = new WiFiManager();
    wifiMgr->begin();

    // ── Restore transport ────────────────────────────────────────────
    int savedTransport = cfg.getLastTransport();
    if (savedTransport >= TRANSPORT_BLE && savedTransport <= TRANSPORT_REMOTE) {
        stats.transport = static_cast<TransportMode>(savedTransport);
    }

    // ── Pre-cache slow stats ─────────────────────────────────────────
    stats.macAddress = WiFi.macAddress();
    stats.version    = "1.0.0";
}

void DeviceContext::loop() {
    // ── Motor PWM ────────────────────────────────────────────────────
    analogWrite(MOTOR_PWM_PIN, map(stats.intensity, 0, 100, 0, 255));

    // ── LED tracks BLE connection ────────────────────────────────────
    static bool lastLed = false;
    if (stats.isBluetoothConnected != lastLed) {
        lastLed = stats.isBluetoothConnected;
        digitalWrite(LED_PIN, lastLed ? HIGH : LOW);
    }

    // ── Subsystem ticks ──────────────────────────────────────────────
    if (wifiMgr) wifiMgr->loop();

    // ── Pending status broadcast ─────────────────────────────────────
    if (statusBroadcastRequested) {
        statusBroadcastRequested = false;
        broadcastStats();
    }
    // delay(10) removed for higher responsiveness
}

// ── State ────────────────────────────────────────────────────────────

DeviceStats& DeviceContext::getStats() { return stats; }

TransportMode DeviceContext::getTransport() const { return stats.transport; }

void DeviceContext::setTransport(TransportMode mode) {
    if (stats.transport == mode) return;

    TransportMode old = stats.transport;
    stats.transport = mode;

    if (wifiMgr) wifiMgr->handleTransportChange(old, mode);

    // Persist
    ConfigManager::getInstance().setLastTransport(static_cast<int>(mode));
}

// ── Subsystem access ─────────────────────────────────────────────────

WiFiManager* DeviceContext::getWiFiManager() { return wifiMgr; }
BLEManager*  DeviceContext::getBLEManager()  { return bleMgr;  }

// ── Lifecycle events ─────────────────────────────────────────────────

void DeviceContext::onWiFiConnected() {
    stats.isWifiConnected = true;
    stats.ipAddress       = WiFi.localIP().toString();
    Serial.print("WiFi connected – IP: ");
    Serial.println(stats.ipAddress);
}

void DeviceContext::onWiFiDisconnected() {
    stats.isWifiConnected = false;
    stats.ipAddress       = "";
    Serial.println("WiFi disconnected");
}

void DeviceContext::onBLEConnected() {
    stats.isBluetoothConnected = true;
    Serial.println("BLE client connected");
}

void DeviceContext::onBLEDisconnected() {
    stats.isBluetoothConnected = false;
    Serial.println("BLE client disconnected");
}

// ── Stats ────────────────────────────────────────────────────────────

void DeviceContext::requestStatusBroadcast() {
    statusBroadcastRequested = true;
}

void DeviceContext::refreshDeviceStats() {
    stats.isWifiConnected = (WiFi.status() == WL_CONNECTED);
    stats.ipAddress       = stats.isWifiConnected ? WiFi.localIP().toString() : String("");
    // macAddress is cached in setup()

    stats.battery    = 100;
    stats.isCharging = false;
}

String DeviceContext::buildStatusJson() const {
    JsonDocument doc;
    doc["intensity"]             = stats.intensity;
    doc["battery"]               = stats.battery;
    doc["isCharging"]            = stats.isCharging;
    doc["isBluetoothConnected"]  = stats.isBluetoothConnected;
    doc["isWifiConnected"]       = stats.isWifiConnected;
    doc["ipAddress"]             = stats.ipAddress;
    doc["macAddress"]            = stats.macAddress;
    doc["version"]               = stats.version;
    doc["deviceId"]              = String((uint32_t)ESP.getEfuseMac(), HEX);

    switch (stats.transport) {
        case TRANSPORT_WIFI:   doc["transport"] = "WIFI";   break;
        case TRANSPORT_REMOTE: doc["transport"] = "REMOTE"; break;
        default:               doc["transport"] = "BLE";    break;
    }

    if (stats.transport == TRANSPORT_REMOTE && !stats.serverAddress.isEmpty()) {
        doc["serverAddress"] = stats.serverAddress;
    }

    String out;
    serializeJson(doc, out);
    Serial.println(out);
    return out;
}

void DeviceContext::broadcastStats() {
    refreshDeviceStats();
    String json = buildStatusJson();

    if (stats.isBluetoothConnected && bleMgr) {
        bleMgr->updateStats(json);
    }

    if (stats.transport != TRANSPORT_BLE && wifiMgr) {
        wifiMgr->sendStats(json);
    }
}
