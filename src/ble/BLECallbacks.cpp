#include "BLECallbacks.h"
#include "../../include/types/device_stats.h"
#include <BLEDevice.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Arduino.h>

void updateAndSendStats();

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

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, value.c_str());
  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    return;
  }

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
    updateAndSendStats();
  } else {
    Serial.println("\nFailed to connect to Wi-Fi.");
  }
}
