#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WebSocketsServer.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "../../include/types/device_stats.h"   // TransportMode only

/**
 * Manages WiFi connectivity and WebSocket communication.
 *
 * Key design decisions:
 *  - Non-blocking: connect() returns immediately; handleWiFiState()
 *    polls WiFi.status() on each loop() tick.
 *  - No globals: reads/writes go through DeviceContext singleton.
 *  - Static wrapper pattern for C-style WebSocket callbacks.
 */
class WiFiManager {
public:
    WiFiManager();
    void begin();
    void loop();

    // WiFi connection (non-blocking)
    void connect();
    void disconnect();
    bool isWiFiConnected() const;

    // Transport change hook
    void handleTransportChange(TransportMode oldMode, TransportMode newMode);

    // WebSocket server (local clients via TRANSPORT_WIFI)
    void startWebSocketServer();
    void stopWebSocketServer();

    // WebSocket client (remote server via TRANSPORT_REMOTE)
    void connectToRemote();
    void disconnectRemote();
    bool isRemoteConnected() const;

    // Send data on whichever transport is active
    void sendStats(String& json);

private:
    // ── WiFi state machine ───────────────────────────────────────────
    enum WiFiState {
        WIFI_IDLE,
        WIFI_CONNECTING,
        WIFI_CONNECTED,
        WIFI_CONNECTION_FAILED,
        WIFI_DISCONNECTED
    };
    WiFiState     wifiState;
    unsigned long wifiStateStart;
    static constexpr unsigned long CONNECT_TIMEOUT_MS = 15000;

    void updateWiFiState(WiFiState s);
    void handleWiFiState();

    // ── WebSocket server ─────────────────────────────────────────────
    WebSocketsServer* wsServer;
    bool wsServerHasClient;

    static void wsServerEventWrapper(uint8_t num, WStype_t type, uint8_t* payload, size_t len);
    void onWsServerEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len);

    // ── WebSocket client (remote) ────────────────────────────────────
    WebSocketsClient* wsClient;
    bool wsClientConnected;
    unsigned long lastRemoteRetry;
    int           remoteRetryCount;
    static constexpr int           MAX_REMOTE_RETRIES  = 100;
    static constexpr unsigned long REMOTE_RETRY_MS     = 15000;

    static void wsClientEventWrapper(WStype_t type, uint8_t* payload, size_t len);
    void onWsClientEvent(WStype_t type, uint8_t* payload, size_t len);
    void retryRemoteIfNeeded();

    // ── Message handling ─────────────────────────────────────────────
    void processIncomingJson(uint8_t* payload);

    // Singleton pointer for C-callback routing
    static WiFiManager* instance;
};

#endif // WIFI_MANAGER_H