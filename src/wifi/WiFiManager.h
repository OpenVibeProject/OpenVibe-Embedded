#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WebSocketsServer.h>
#include <ArduinoJson.h>

class WiFiManager {
public:
    WiFiManager();
    void begin();
    void loop();
    void sendStats(String& statsJson);
    bool isWebSocketConnected();

private:
    WebSocketsServer* webSocket;
    bool wsConnected;
    void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    static void webSocketEventWrapper(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    static WiFiManager* instance;
};

#endif // WIFI_MANAGER_H