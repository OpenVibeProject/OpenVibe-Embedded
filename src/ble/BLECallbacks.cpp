#include "BLECallbacks.h"
#include "../../include/types/device_stats.h"
#include "../wifi/WiFiCredentials.h"
#include <BLEDevice.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Arduino.h>

void updateAndSendStats();
void sendStatusWithNewTransport(TransportMode newTransport, const String& serverAddr = "");

void BLEServerHandler::onConnect(BLEServer* server) {
  deviceConnected = true;
  Serial.println("BLE Client connected!");
}

void BLEServerHandler::onDisconnect(BLEServer* server) {
  deviceConnected = false;
  Serial.println("BLE Client disconnected!");
}

void WiFiConfigCharacteristicHandler::onWrite(BLECharacteristic* characteristic) {
  std::string value = characteristic->getValue();
  if (value.length() == 0) return;

  Serial.print("Received via BLE: ");
  Serial.println(value.c_str());

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, value.c_str());
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
  } else if (strcmp(requestType, "WIFI_CREDENTIALS") == 0) {
    const char* ssid = doc["ssid"];
    const char* password = doc["password"];
    Serial.print("Connecting to Wi-Fi SSID: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWi-Fi connected!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      WiFiCredentials::save(ssid, password);
      extern bool statusRequested;
      statusRequested = true;
    } else {
      Serial.println("\nFailed to connect to Wi-Fi.");
    }
  } else if (strcmp(requestType, "SWITCH_TRANSPORT") == 0) {
    extern DeviceStats deviceStats;
    
    const char* transport = doc["transport"];
    TransportMode newTransport = deviceStats.transport;
    String serverAddr = "";
    
    if (strcmp(transport, "BLE") == 0) {
      newTransport = TRANSPORT_BLE;
    } else if (strcmp(transport, "WIFI") == 0) {
      newTransport = TRANSPORT_WIFI;
    } else if (strcmp(transport, "REMOTE") == 0) {
      newTransport = TRANSPORT_REMOTE;
      const char* serverAddress = doc["serverAddress"];
      if (serverAddress) {
        serverAddr = String(serverAddress);
      }
    }
    
    sendStatusWithNewTransport(newTransport, serverAddr);
    
    deviceStats.transport = newTransport;
    if (!serverAddr.isEmpty()) {
      deviceStats.serverAddress = serverAddr;
    }
    Serial.print("Switched to transport: ");
    Serial.println(newTransport);
  }
}
