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
#include "wifi/WiFiManager.h"
#include "wifi/WiFiCredentials.h"

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
WiFiManager wifiManager;

volatile bool deviceConnected = false;
bool statusRequested = false;

void updateAndSendStats();
void handleTransportChange();

bool isBLEConnected() { return deviceConnected; }

TransportMode lastTransportMode = TRANSPORT_BLE;

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  WiFi.mode(WIFI_STA);

  String ssid, password;
  if (WiFiCredentials::load(ssid, password)) {
    Serial.println("Connecting to saved WiFi...");
    WiFi.begin(ssid.c_str(), password.c_str());
  }

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

  deviceStats.battery = random(0, 101);
  deviceStats.isCharging = false;

  DynamicJsonDocument doc(512);
  unsigned long uptime = millis();
  doc["intensity"] = deviceStats.intensity;
  doc["battery"] = deviceStats.battery;
  doc["isCharging"] = deviceStats.isCharging;
  doc["isBluetoothConnected"] = deviceStats.isBluetoothConnected;
  doc["isWifiConnected"] = deviceStats.isWifiConnected;
  doc["ipAddress"] = deviceStats.ipAddress;
  doc["macAddress"] = deviceStats.macAddress;
  doc["version"] = deviceStats.version;
  doc["deviceId"] = String((uint32_t)ESP.getEfuseMac(), HEX);
  
  // Add transport mode to stats
  const char* transportStr = "BLE";
  if (deviceStats.transport == TRANSPORT_WIFI) transportStr = "WIFI";
  else if (deviceStats.transport == TRANSPORT_REMOTE) transportStr = "REMOTE";
  doc["transport"] = transportStr;
  
  if (deviceStats.transport == TRANSPORT_REMOTE && !deviceStats.serverAddress.isEmpty()) {
    doc["serverAddress"] = deviceStats.serverAddress;
  }

  String jsonString;
  serializeJson(doc, jsonString);

  String jsonCopy = jsonString;
  
  // Send stats based on current transport mode
  if (deviceStats.transport == TRANSPORT_REMOTE && wifiManager.isRemoteConnected()) {
    wifiManager.sendStats(jsonCopy);
    Serial.println("Stats sent via Remote WebSocket: " + jsonString);
  } else if (deviceStats.transport == TRANSPORT_WIFI && wifiManager.isWebSocketConnected()) {
    wifiManager.sendStats(jsonCopy);
    Serial.println("Stats sent via WiFi WebSocket: " + jsonString);
  } else if (deviceStats.transport == TRANSPORT_BLE && deviceConnected && pStatsCharacteristic != nullptr) {
    pStatsCharacteristic->setValue(jsonString.c_str());
    pStatsCharacteristic->notify();
    Serial.println("Stats sent via BLE: " + jsonString);
  }
}

void loop() {
  static bool lastLedState = false;
  static bool lastConnectionState = false;
  static bool wifiManagerStarted = false;
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

  if (WiFi.status() == WL_CONNECTED && !wifiManagerStarted) {
    wifiManager.begin();
    wifiManagerStarted = true;
    
    // Only start WebSocket server if transport mode is WIFI
    if (deviceStats.transport == TRANSPORT_WIFI) {
      wifiManager.startWebSocketServer();
    }
  }

  wifiManager.loop();

  if (statusRequested) {
    statusRequested = false;
    updateAndSendStats();
  }

  handleTransportChange();
  
  delay(10);
}

void handleTransportChange() {
  if (deviceStats.transport != lastTransportMode) {
    Serial.print("Transport mode changed from ");
    Serial.print(lastTransportMode);
    Serial.print(" to ");
    Serial.println(deviceStats.transport);
    
    // Close previous connections
    if (lastTransportMode == TRANSPORT_WIFI) {
      wifiManager.stopWebSocketServer();
    } else if (lastTransportMode == TRANSPORT_REMOTE) {
      wifiManager.disconnectRemote();
    }
    
    // Start new connections based on transport mode
    if (deviceStats.transport == TRANSPORT_WIFI && WiFi.status() == WL_CONNECTED) {
      wifiManager.startWebSocketServer();
    } else if (deviceStats.transport == TRANSPORT_REMOTE && !deviceStats.serverAddress.isEmpty()) {
      wifiManager.connectToRemote(deviceStats.serverAddress);
    }
    
    lastTransportMode = deviceStats.transport;
  }
}