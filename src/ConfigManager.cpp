#include "ConfigManager.h"

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

// ── WiFi ──────────────────────────────────────────────────────────────

void ConfigManager::setWiFiCredentials(const String& ssid, const String& password) {
    Preferences p;
    p.begin(NS, false);
    p.putString("wifi_ssid", ssid);
    p.putString("wifi_pass", password);
    p.end();
}

String ConfigManager::getWiFiSSID() {
    Preferences p;
    p.begin(NS, true);
    String v = p.getString("wifi_ssid", "");
    p.end();
    return v;
}

String ConfigManager::getWiFiPassword() {
    Preferences p;
    p.begin(NS, true);
    String v = p.getString("wifi_pass", "");
    p.end();
    return v;
}

bool ConfigManager::hasWiFiCredentials() {
    return !getWiFiSSID().isEmpty();
}

void ConfigManager::clearWiFiCredentials() {
    Preferences p;
    p.begin(NS, false);
    p.remove("wifi_ssid");
    p.remove("wifi_pass");
    p.end();
}

// ── Device ────────────────────────────────────────────────────────────

void ConfigManager::setDeviceName(const String& name) {
    Preferences p;
    p.begin(NS, false);
    p.putString("dev_name", name);
    p.end();
}

String ConfigManager::getDeviceName() {
    Preferences p;
    p.begin(NS, true);
    String v = p.getString("dev_name", "OpenVibe");
    p.end();
    return v;
}

// ── Transport ─────────────────────────────────────────────────────────

void ConfigManager::setLastTransport(int transport) {
    Preferences p;
    p.begin(NS, false);
    p.putInt("transport", transport);
    p.end();
}

int ConfigManager::getLastTransport() {
    Preferences p;
    p.begin(NS, true);
    int v = p.getInt("transport", 0);
    p.end();
    return v;
}

// ── Remote server ─────────────────────────────────────────────────────

void ConfigManager::setRemoteServer(const String& url) {
    Preferences p;
    p.begin(NS, false);
    p.putString("remote_url", url);
    p.end();
}

String ConfigManager::getRemoteServer() {
    Preferences p;
    p.begin(NS, true);
    String v = p.getString("remote_url", "");
    p.end();
    return v;
}
