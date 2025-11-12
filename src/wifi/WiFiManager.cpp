#include "WiFiManager.h"
#include "../../include/types/device_stats.h"
#include <WiFi.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

WiFiManager* WiFiManager::instance = nullptr;

WiFiManager::WiFiManager() : webSocket(nullptr), wsClient(nullptr), wsConnected(false), wsClientConnected(false), lastRetryTime(0), retryCount(0) {
    instance = this;
}

void WiFiManager::begin() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    webSocket = new WebSocketsServer(6969);
    webSocket->begin();
    webSocket->onEvent(webSocketEventWrapper);
    webSocket->enableHeartbeat(15000, 5000, 3);
    
    Serial.println("WebSocket server started on port 6969");
    Serial.print("WebSocket URL: ws://");
    Serial.print(WiFi.localIP());
    Serial.println(":6969");
}

void WiFiManager::loop() {
    if (webSocket) {
        webSocket->loop();
    }
    if (wsClient) {
        wsClient->loop();
    }
    retryRemoteConnection();
}

void WiFiManager::sendStats(String& statsJson) {
    extern DeviceStats deviceStats;
    
    if (deviceStats.transport == TRANSPORT_REMOTE && wsClient && wsClientConnected) {
        wsClient->sendTXT(statsJson);
    } else if (webSocket && wsConnected) {
        webSocket->broadcastTXT(statsJson);
    }
}

bool WiFiManager::isWebSocketConnected() {
    extern DeviceStats deviceStats;
    
    if (deviceStats.transport == TRANSPORT_REMOTE) {
        return wsClientConnected;
    }
    return wsConnected;
}

bool WiFiManager::isRemoteConnected() {
    return wsClientConnected;
}

void WiFiManager::connectToRemote(const String& url) {
    if (wsClient) {
        delete wsClient;
        wsClient = nullptr;
        wsClientConnected = false;
    }
    
    if (url.isEmpty()) {
        Serial.println("Remote URL is empty");
        return;
    }
    
    serverAddress = url;
    retryCount = 0;
    lastRetryTime = 0;
    
    wsClient = new WebSocketsClient();
    
    // Parse URL - simple implementation for ws://host:port format
    String host;
    int port = 80;
    String path = "/";
    
    if (url.startsWith("ws://")) {
        String urlPart = url.substring(5); // Remove "ws://"
        int colonIndex = urlPart.indexOf(':');
        int slashIndex = urlPart.indexOf('/');
        
        if (colonIndex > 0) {
            host = urlPart.substring(0, colonIndex);
            if (slashIndex > 0) {
                port = urlPart.substring(colonIndex + 1, slashIndex).toInt();
                path = urlPart.substring(slashIndex);
            } else {
                port = urlPart.substring(colonIndex + 1).toInt();
            }
        } else {
            if (slashIndex > 0) {
                host = urlPart.substring(0, slashIndex);
                path = urlPart.substring(slashIndex);
            } else {
                host = urlPart;
            }
        }
    } else {
        Serial.println("Invalid WebSocket URL format. Expected ws://host:port/path");
        delete wsClient;
        wsClient = nullptr;
        return;
    }
    
    if (host.isEmpty()) {
        Serial.println("Invalid host in WebSocket URL");
        delete wsClient;
        wsClient = nullptr;
        return;
    }
    
    wsClient->begin(host, port, path);
    wsClient->onEvent(webSocketClientEventWrapper);
    wsClient->enableHeartbeat(15000, 3000, 2);
    
    Serial.print("Connecting to remote WebSocket: ");
    Serial.print(host);
    Serial.print(":");
    Serial.print(port);
    Serial.println(path);
}

void WiFiManager::webSocketEventWrapper(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (instance) {
        instance->onWebSocketEvent(num, type, payload, length);
    }
}

void WiFiManager::webSocketClientEventWrapper(WStype_t type, uint8_t* payload, size_t length) {
    if (instance) {
        instance->onWebSocketClientEvent(type, payload, length);
    }
}

void WiFiManager::onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            wsConnected = false;
            Serial.printf("WebSocket client %u disconnected\n", num);
            break;
        case WStype_CONNECTED:
            wsConnected = true;
            Serial.printf("WebSocket client %u connected from %s\n", num, webSocket->remoteIP(num).toString().c_str());
            break;
        case WStype_TEXT: {
            Serial.printf("WebSocket received: %s\n", payload);
            
            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, (char*)payload);
            if (error) {
                Serial.print("JSON parse failed: ");
                Serial.println(error.c_str());
                return;
            }
            
            const char* requestType = doc["requestType"];
            if (!requestType) return;
            
            if (strcmp(requestType, "STATUS") == 0) {
                extern bool statusRequested;
                statusRequested = true;
            } else if (strcmp(requestType, "INTENSITY") == 0) {
                extern DeviceStats deviceStats;
                deviceStats.intensity = doc["intensity"];
                Serial.print("Intensity set to: ");
                Serial.println(deviceStats.intensity);
            } else if (strcmp(requestType, "SWITCH_TRANSPORT") == 0) {
                extern DeviceStats deviceStats;
                const char* transport = doc["transport"];
                
                if (strcmp(transport, "BLE") == 0) {
                    deviceStats.transport = TRANSPORT_BLE;
                    Serial.println("Switched to BLE transport");
                } else if (strcmp(transport, "WIFI") == 0) {
                    deviceStats.transport = TRANSPORT_WIFI;
                    Serial.println("Switched to WIFI transport");
                } else if (strcmp(transport, "REMOTE") == 0) {
                    deviceStats.transport = TRANSPORT_REMOTE;
                    const char* serverAddress = doc["serverAddress"];
                    if (serverAddress) {
                        deviceStats.serverAddress = String(serverAddress);
                        Serial.print("Switched to REMOTE transport: ");
                        Serial.println(deviceStats.serverAddress);
                    }
                }
                
                extern bool statusRequested;
                statusRequested = true;
            }
            break;
        }
        default:
            break;
    }
}

void WiFiManager::onWebSocketClientEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            wsClientConnected = false;
            Serial.println("Remote WebSocket disconnected");
            if (retryCount < MAX_RETRIES) {
                lastRetryTime = millis();
                Serial.printf("Will retry in %lu seconds (attempt %d/%d)\n", RETRY_INTERVAL/1000, retryCount+1, MAX_RETRIES);
            }
            break;
        case WStype_CONNECTED:
            wsClientConnected = true;
            retryCount = 0;
            Serial.println("Remote WebSocket connected");
            break;
        case WStype_TEXT:
            Serial.printf("Remote WebSocket received: %s\n", payload);
            break;
        default:
            break;
    }
}

void WiFiManager::retryRemoteConnection() {
    extern DeviceStats deviceStats;
    
    if (deviceStats.transport != TRANSPORT_REMOTE || serverAddress.isEmpty() || wsClientConnected) {
        return;
    }
    
    if (retryCount >= MAX_RETRIES) {
        return;
    }
    
    if (lastRetryTime > 0 && (millis() - lastRetryTime) >= RETRY_INTERVAL) {
        retryCount++;
        Serial.printf("Retrying remote WebSocket connection (attempt %d/%d)\n", retryCount, MAX_RETRIES);
        connectToRemote(serverAddress);
    }
}