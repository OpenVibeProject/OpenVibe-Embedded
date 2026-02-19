#include "WiFiManager.h"
#include "../DeviceContext.h"
#include "../ConfigManager.h"
#include <WiFi.h>

WiFiManager* WiFiManager::instance = nullptr;

// ── Constructor ──────────────────────────────────────────────────────

WiFiManager::WiFiManager()
    : wifiState(WIFI_IDLE)
    , wifiStateStart(0)
    , wsServer(nullptr)
    , wsServerHasClient(false)
    , wsClient(nullptr)
    , wsClientConnected(false)
    , lastRemoteRetry(0)
    , remoteRetryCount(0)
{
    instance = this;
}

// ── Lifecycle ────────────────────────────────────────────────────────

void WiFiManager::begin() {
    WiFi.mode(WIFI_STA);

    if (ConfigManager::getInstance().hasWiFiCredentials()) {
        connect();
    }
}

void WiFiManager::loop() {
    handleWiFiState();

    if (wsServer) wsServer->loop();
    if (wsClient) wsClient->loop();

    retryRemoteIfNeeded();
}

// ── WiFi connection (non-blocking) ───────────────────────────────────

void WiFiManager::connect() {
    ConfigManager& cfg = ConfigManager::getInstance();
    String ssid = cfg.getWiFiSSID();
    String pass = cfg.getWiFiPassword();

    if (ssid.isEmpty()) {
        Serial.println("[WiFi] No credentials stored");
        updateWiFiState(WIFI_DISCONNECTED);
        return;
    }

    Serial.printf("[WiFi] Connecting to \"%s\"...\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());
    updateWiFiState(WIFI_CONNECTING);
}

void WiFiManager::disconnect() {
    WiFi.disconnect();
    stopWebSocketServer();
    disconnectRemote();
    updateWiFiState(WIFI_DISCONNECTED);
}

bool WiFiManager::isWiFiConnected() const {
    return wifiState == WIFI_CONNECTED;
}

void WiFiManager::updateWiFiState(WiFiState s) {
    wifiState      = s;
    wifiStateStart = millis();

    DeviceContext& ctx = DeviceContext::getInstance();

    switch (s) {
        case WIFI_CONNECTED:
            ctx.onWiFiConnected();
            // Auto-start appropriate transport
            if (ctx.getTransport() == TRANSPORT_WIFI)   startWebSocketServer();
            if (ctx.getTransport() == TRANSPORT_REMOTE)  connectToRemote();
            break;

        case WIFI_DISCONNECTED:
        case WIFI_CONNECTION_FAILED:
            ctx.onWiFiDisconnected();
            break;

        default:
            break;
    }
}

void WiFiManager::handleWiFiState() {
    switch (wifiState) {
        case WIFI_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                updateWiFiState(WIFI_CONNECTED);
            } else if (millis() - wifiStateStart > CONNECT_TIMEOUT_MS) {
                Serial.println("[WiFi] Connection timeout");
                updateWiFiState(WIFI_CONNECTION_FAILED);
            }
            break;

        case WIFI_CONNECTED:
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[WiFi] Connection lost — reconnecting");
                updateWiFiState(WIFI_DISCONNECTED);
                connect();
            }
            break;

        default:
            break;
    }
}

// ── Transport change ─────────────────────────────────────────────────

void WiFiManager::handleTransportChange(TransportMode oldMode, TransportMode newMode) {
    // Tear down old transport channel
    if (oldMode == TRANSPORT_WIFI)   stopWebSocketServer();
    if (oldMode == TRANSPORT_REMOTE) disconnectRemote();

    // Bring up new one (only if WiFi is already connected)
    if (wifiState != WIFI_CONNECTED) return;
    if (newMode == TRANSPORT_WIFI)   startWebSocketServer();
    if (newMode == TRANSPORT_REMOTE) connectToRemote();
}

// ── WebSocket server ─────────────────────────────────────────────────

void WiFiManager::startWebSocketServer() {
    if (wsServer) return;

    wsServer = new WebSocketsServer(6969);
    wsServer->begin();
    wsServer->onEvent(wsServerEventWrapper);

    Serial.printf("[WS-Server] Listening on ws://%s:6969\n",
                  WiFi.localIP().toString().c_str());
}

void WiFiManager::stopWebSocketServer() {
    if (!wsServer) return;
    wsServer->close();
    delete wsServer;
    wsServer          = nullptr;
    wsServerHasClient = false;
}

void WiFiManager::wsServerEventWrapper(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
    if (instance) instance->onWsServerEvent(num, type, payload, len);
}

void WiFiManager::onWsServerEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
    switch (type) {
        case WStype_CONNECTED:
            wsServerHasClient = true;
            Serial.printf("[WS-Server] Client #%u connected\n", num);
            break;
        case WStype_DISCONNECTED:
            wsServerHasClient = false;
            Serial.printf("[WS-Server] Client #%u disconnected\n", num);
            break;
        case WStype_TEXT:
            processIncomingJson(payload);
            break;
        default: break;
    }
}

// ── WebSocket client (remote) ────────────────────────────────────────

void WiFiManager::connectToRemote() {
    ConfigManager& cfg = ConfigManager::getInstance();
    String url = cfg.getRemoteServer();

    if (url.isEmpty()) {
        Serial.println("[WS-Client] No remote URL configured");
        return;
    }

    disconnectRemote();

    // ── Parse ws://host:port/path ────────────────────────────────────
    String host;
    int    port = 80;
    String path = "/";

    if (!url.startsWith("ws://")) {
        Serial.println("[WS-Client] Invalid URL (expected ws://)");
        return;
    }

    String body = url.substring(5);
    int colon = body.indexOf(':');
    int slash = body.indexOf('/');

    if (colon > 0) {
        host = body.substring(0, colon);
        port = (slash > colon)
             ? body.substring(colon + 1, slash).toInt()
             : body.substring(colon + 1).toInt();
        if (slash > 0) path = body.substring(slash);
    } else {
        host = (slash > 0) ? body.substring(0, slash) : body;
        if (slash > 0) path = body.substring(slash);
    }

    // Append device registration path
    String deviceId = String((uint32_t)ESP.getEfuseMac(), HEX);
    if (!path.endsWith("/")) path += "/";
    path += "register?id=" + deviceId;

    wsClient = new WebSocketsClient();
    wsClient->begin(host, port, path);
    wsClient->onEvent(wsClientEventWrapper);
    lastRemoteRetry  = millis();
    remoteRetryCount = 0;

    Serial.printf("[WS-Client] Connecting to %s:%d%s\n",
                  host.c_str(), port, path.c_str());
}

void WiFiManager::disconnectRemote() {
    if (!wsClient) return;
    delete wsClient;
    wsClient          = nullptr;
    wsClientConnected = false;
    remoteRetryCount  = 0;
}

bool WiFiManager::isRemoteConnected() const {
    return wsClientConnected;
}

void WiFiManager::wsClientEventWrapper(WStype_t type, uint8_t* payload, size_t len) {
    if (instance) instance->onWsClientEvent(type, payload, len);
}

void WiFiManager::onWsClientEvent(WStype_t type, uint8_t* payload, size_t len) {
    switch (type) {
        case WStype_CONNECTED:
            wsClientConnected = true;
            remoteRetryCount  = 0;
            Serial.println("[WS-Client] Connected to remote");
            break;

        case WStype_DISCONNECTED:
            wsClientConnected = false;
            lastRemoteRetry   = millis();
            Serial.println("[WS-Client] Disconnected from remote");
            break;

        case WStype_TEXT:
            processIncomingJson(payload);
            break;

        default: break;
    }
}

void WiFiManager::retryRemoteIfNeeded() {
    DeviceContext& ctx = DeviceContext::getInstance();

    if (ctx.getTransport() != TRANSPORT_REMOTE) return;
    if (wifiState != WIFI_CONNECTED)             return;
    if (wsClientConnected)                        return;
    if (remoteRetryCount >= MAX_REMOTE_RETRIES)   return;
    if (millis() - lastRemoteRetry < REMOTE_RETRY_MS) return;

    remoteRetryCount++;
    Serial.printf("[WS-Client] Retry %d/%d\n", remoteRetryCount, MAX_REMOTE_RETRIES);
    connectToRemote();
}

// ── Send ─────────────────────────────────────────────────────────────

void WiFiManager::sendStats(String& json) {
    if (wsServer && wsServerHasClient) {
        wsServer->broadcastTXT(json);
    }
    if (wsClient && wsClientConnected) {
        wsClient->sendTXT(json);
    }
}

// ── Incoming JSON ────────────────────────────────────────────────────

void WiFiManager::processIncomingJson(uint8_t* payload) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, (char*)payload);
    if (err) {
        Serial.printf("[WS] JSON parse error: %s\n", err.c_str());
        return;
    }

    DeviceContext& ctx    = DeviceContext::getInstance();
    ConfigManager& cfg    = ConfigManager::getInstance();
    const char* reqType   = doc["requestType"];

    if (!reqType) return;

    if (strcmp(reqType, "STATUS") == 0) {
        ctx.requestStatusBroadcast();

    } else if (strcmp(reqType, "INTENSITY") == 0) {
        int val = doc["intensity"].as<int>();
        ctx.getStats().intensity = constrain(val, 0, 100);
        Serial.printf("[WS] Intensity → %d\n", ctx.getStats().intensity);

    } else if (strcmp(reqType, "SWITCH_TRANSPORT") == 0) {
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
        Serial.printf("[WS] Transport → %s\n", t);

    } else if (strcmp(reqType, "WIFI_CREDENTIALS") == 0) {
        const char* ssid = doc["ssid"];
        const char* pass = doc["password"];
        if (ssid) {
            cfg.setWiFiCredentials(ssid, pass ? pass : "");
            connect();
        }
    }
}