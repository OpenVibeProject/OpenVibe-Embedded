#include "WiFiManager.h"
#include "../../include/types/device_stats.h"
#include <WiFi.h>
#include <Arduino.h>
#include <ArduinoJson.h>

WiFiManager* WiFiManager::instance = nullptr;

WiFiManager::WiFiManager() : webSocket(nullptr), wsConnected(false) {
    instance = this;
}

void WiFiManager::begin() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    webSocket = new WebSocketsServer(6969);
    webSocket->begin();
    webSocket->onEvent(webSocketEventWrapper);
    webSocket->enableHeartbeat(15000, 3000, 2);
    
    Serial.println("WebSocket server started on port 6969");
    Serial.print("WebSocket URL: ws://");
    Serial.print(WiFi.localIP());
    Serial.println(":6969");
}

void WiFiManager::loop() {
    if (webSocket) {
        webSocket->loop();
    }
}

void WiFiManager::sendStats(String& statsJson) {
    if (webSocket && wsConnected) {
        webSocket->broadcastTXT(statsJson);
    }
}

bool WiFiManager::isWebSocketConnected() {
    return wsConnected;
}

void WiFiManager::webSocketEventWrapper(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (instance) {
        instance->onWebSocketEvent(num, type, payload, length);
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
            }
            break;
        }
        default:
            break;
    }
}