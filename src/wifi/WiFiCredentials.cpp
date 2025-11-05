#include "WiFiCredentials.h"

void WiFiCredentials::save(const char* ssid, const char* password) {
    Preferences prefs;
    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.end();
}

bool WiFiCredentials::load(String& ssid, String& password) {
    Preferences prefs;
    prefs.begin("wifi", true);
    ssid = prefs.getString("ssid", "");
    password = prefs.getString("password", "");
    prefs.end();
    return ssid.length() > 0;
}

void WiFiCredentials::clear() {
    Preferences prefs;
    prefs.begin("wifi", false);
    prefs.clear();
    prefs.end();
}