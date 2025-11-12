#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WebSocketsServer.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

class WiFiManager {
public:
    WiFiManager();
    void begin();
    void loop();
    void sendStats(String& statsJson);
    bool isWebSocketConnected();
    void connectToRemote(const String& url);
    bool isRemoteConnected();

private:
    WebSocketsServer* webSocket;
    WebSocketsClient* wsClient;
    bool wsConnected;
    bool wsClientConnected;
    String serverAddress;
    unsigned long lastRetryTime;
    int retryCount;
    static const int MAX_RETRIES = 5;
    static const unsigned long RETRY_INTERVAL = 10000;
    void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    void onWebSocketClientEvent(WStype_t type, uint8_t* payload, size_t length);
    void retryRemoteConnection();
    static void webSocketEventWrapper(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    static void webSocketClientEventWrapper(WStype_t type, uint8_t* payload, size_t length);
    static WiFiManager* instance;
};

#endif // WIFI_MANAGER_H