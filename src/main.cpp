#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSockets.h>
#include <string>
#include <base64.h>

#include "types/device_stats.h"
#include "ble/BLECallbacks.h"

#define FIRMWARE_VERSION "1.0.0"
// Firmware version and UUIDs
#define SERVICE_UUID "ec2e0883-782d-433b-9a0c-6d5df5565410"
#define WIFI_CHAR_UUID "c2433dd7-137e-4e82-845e-a40f70dc4a8d"
#define STATS_CHAR_UUID "c2433dd7-137e-4e82-845e-a40f70dc4a8e"

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pWiFiCharacteristic;
BLECharacteristic *pStatsCharacteristic;

DeviceStats deviceStats;

volatile bool deviceConnected = false;

void updateAndSendStats();

bool isBLEConnected() { return deviceConnected; }

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  WiFi.mode(WIFI_STA);

  String device_name("OpenVibe");
  String device_id(base64::encode(WiFi.macAddress()));
  String full_device_name = device_name + device_id;
  BLEDevice::init(full_device_name.c_str());

  pServer = BLEDevice::createServer();
  (*pServer).setCallbacks(new BLEServerHandler());

  pService = (*pServer).createService(SERVICE_UUID);

  pWiFiCharacteristic = (*pService).createCharacteristic(WIFI_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
  (*pWiFiCharacteristic).setCallbacks(new WiFiConfigCharacteristicHandler());

  pStatsCharacteristic = (*pService).createCharacteristic(STATS_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);

  BLEDescriptor* pDesc = new BLEDescriptor(BLEUUID((uint16_t)0x2902));
  pStatsCharacteristic->addDescriptor(pDesc);

  (*pService).start();
  BLEDevice::startAdvertising();
}

void updateAndSendStats() {
  deviceStats.isBluetoothConnected = deviceConnected;
  deviceStats.isWifiConnected = (WiFi.status() == WL_CONNECTED);
  deviceStats.macAddress = WiFi.macAddress();
  deviceStats.ipAddress = deviceStats.isWifiConnected ? WiFi.localIP().toString() : "";

  deviceStats.battery = 100;
  deviceStats.isCharging = false;
  deviceStats.intensity = 50;

  DynamicJsonDocument doc(512);
  unsigned long uptime = millis();
  doc["uptimeMillis"] = uptime;
  doc["intensity"] = deviceStats.intensity;
  doc["battery"] = deviceStats.battery;
  doc["isCharging"] = deviceStats.isCharging;
  doc["isBluetoothConnected"] = deviceStats.isBluetoothConnected;
  doc["isWifiConnected"] = deviceStats.isWifiConnected;
  doc["ipAddress"] = deviceStats.ipAddress;
  doc["macAddress"] = deviceStats.macAddress;
  doc["version"] = deviceStats.version;

  String jsonString;
  serializeJson(doc, jsonString);

  if (deviceConnected && pStatsCharacteristic != nullptr) {
    Serial.println("Attempting to send BLE notification...");
    pStatsCharacteristic->setValue(jsonString.c_str());
    pStatsCharacteristic->notify();
    Serial.println("Stats notification triggered (client must enable notifications to receive it)");
    Serial.println("Stats JSON: " + jsonString);
  } else {
    Serial.println("Not sending stats - deviceConnected: " + String(deviceConnected) + ", pStatsCharacteristic: " + String(pStatsCharacteristic != nullptr));
  }
}

void loop() {
  static bool lastLedState = false;
  static unsigned long lastStatsUpdate = 0;
  static bool lastConnectionState = false;
  bool connected = isBLEConnected();

  if (!connected && lastConnectionState) {
    Serial.println("BLE disconnected, restarting advertising...");
    BLEDevice::startAdvertising();
  }
  lastConnectionState = connected;

  if (connected != lastLedState) {
    lastLedState = connected;
    digitalWrite(2, connected ? HIGH : LOW);
    Serial.println(connected ? "Device connected" : "Device disconnected");
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastStatsUpdate >= 5000) {
    lastStatsUpdate = currentMillis;
    updateAndSendStats();
  }

  delay(100);
}