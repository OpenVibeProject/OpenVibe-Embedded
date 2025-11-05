#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H

#include <Preferences.h>

class WiFiCredentials {
public:
    static void save(const char* ssid, const char* password);
    static bool load(String& ssid, String& password);
    static void clear();
};

#endif // WIFI_CREDENTIALS_H