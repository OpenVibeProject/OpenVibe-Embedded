#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSockets.h>
#include <string>
#include <base64.h>

// Firmware version
#define FIRMWARE_VERSION "1.0.0"

// BLE UUIDs
#define SERVICE_UUID "ec2e0883-782d-433b-9a0c-6d5df5565410"
#define WIFI_CHAR_UUID "c2433dd7-137e-4e82-845e-a40f70dc4a8d"
#define STATS_CHAR_UUID "c2433dd7-137e-4e82-845e-a40f70dc4a8e"

// Global BLE objects
BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pWiFiCharacteristic;    // For WiFi credentials
BLECharacteristic *pStatsCharacteristic;   // For device statistics

// Device state structure
struct DeviceStats {
    int intensity = 0;          // Current intensity level
    int battery = 0;           // Battery percentage
    bool isCharging = false;   // Charging status
    bool isBluetoothConnected = false;
    bool isWifiConnected = false;
    String ipAddress;          // Current IP address or empty string
    String macAddress;         // Device MAC address
    const char* version = FIRMWARE_VERSION;
} deviceStats;

volatile bool deviceConnected = false; // flagged by BLE callbacks

// Forward declaration of device stats update function
void updateAndSendStats();

bool isBLEConnected() {
  return deviceConnected;
}

// Server callbacks
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override {
    deviceConnected = true;
    Serial.println("BLE Client connected!");
  }
  void onDisconnect(BLEServer* server) override {
    deviceConnected = false;
    Serial.println("BLE Client disconnected!");
  }
};

// Characteristic callbacks to log received JSON and connect Wi-Fi
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    std::string value = characteristic->getValue();
    if (value.length() > 0) {
      Serial.print("Received via BLE: ");
      Serial.println(value.c_str());

      // Parse JSON
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, value.c_str());
      if (error) {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
        return;
      }

      // Extract credentials
      const char* ssid = doc["ssid"];
      const char* password = doc["password"];
      Serial.print("Connecting to Wi-Fi SSID: ");
      Serial.println(ssid);

      // Connect to Wi-Fi
      WiFi.begin(ssid, password);

      // Wait for connection (max ~10s)
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
        
        // Send immediate stats update on successful WiFi connection
        updateAndSendStats();
      } else {
        Serial.println("\nFailed to connect to Wi-Fi.");
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  WiFi.mode(WIFI_STA); // Set Wi-Fi to station mode

  String device_name("OpenVibe");
  String device_id(base64::encode(WiFi.macAddress()));
  String full_device_name = device_name + device_id;
  BLEDevice::init(full_device_name.c_str());
  // BLEDevice::init("ESP32-BLE");


  pServer = BLEDevice::createServer();
  (*pServer).setCallbacks(new MyServerCallbacks());

  // Create BLE service
  pService = (*pServer).createService(SERVICE_UUID);
  
  // Create WiFi configuration characteristic (write)
  pWiFiCharacteristic = (*pService).createCharacteristic(
                      WIFI_CHAR_UUID,
                      BLECharacteristic::PROPERTY_WRITE
                    );
  (*pWiFiCharacteristic).setCallbacks(new MyCharacteristicCallbacks());

  // Create device statistics characteristic (notify)
  pStatsCharacteristic = (*pService).createCharacteristic(
                      STATS_CHAR_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
                    );

  // Add descriptor to enable notifications
  BLEDescriptor* pDesc = new BLEDescriptor(BLEUUID((uint16_t)0x2902));
  pStatsCharacteristic->addDescriptor(pDesc);

  // Start service and advertising
  (*pService).start();
  BLEDevice::startAdvertising();
}

// Update device statistics and send via BLE if connected
void updateAndSendStats() {
    // Update device stats
    deviceStats.isBluetoothConnected = deviceConnected;
    deviceStats.isWifiConnected = (WiFi.status() == WL_CONNECTED);
    deviceStats.macAddress = WiFi.macAddress();
    deviceStats.ipAddress = deviceStats.isWifiConnected ? WiFi.localIP().toString() : "";
    
    // TODO: Implement actual readings
    deviceStats.battery = 100;  // Mock battery level
    deviceStats.isCharging = false;  // Mock charging status
    deviceStats.intensity = 50;  // Mock intensity level

    // Create JSON document
    JsonDocument doc;
  // Add current ESP32 uptime in milliseconds
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

    // Serialize to JSON string
    String jsonString;
    serializeJson(doc, jsonString);

    // Send via BLE if connected
    if (deviceConnected && pStatsCharacteristic != nullptr) {
        Serial.println("Attempting to send BLE notification...");
    pStatsCharacteristic->setValue(jsonString.c_str());
    // notify() has void return type in this BLE library, so we can't
    // assign its result. Call it and log that the notification was
    // triggered; the client must enable notifications to actually receive it.
    pStatsCharacteristic->notify();
    Serial.println("Stats notification triggered (client must enable notifications to receive it)");
        Serial.println("Stats JSON: " + jsonString);
    } else {
        Serial.println("Not sending stats - deviceConnected: " + String(deviceConnected) + 
                      ", pStatsCharacteristic: " + String(pStatsCharacteristic != nullptr));
    }
}

void loop() {
  static bool lastLedState = false;
  static unsigned long lastStatsUpdate = 0;
  static bool lastConnectionState = false;
  bool connected = isBLEConnected();

  // Verifica se il BLE è disconnesso e riavvia l'advertising se necessario
  if (!connected && lastConnectionState) {
    Serial.println("BLE disconnected, restarting advertising...");
    BLEDevice::startAdvertising();
  }
  lastConnectionState = connected;

  // Update LED only when state changes
  if (connected != lastLedState) {
    lastLedState = connected;
    digitalWrite(2, connected ? HIGH : LOW);
    Serial.println(connected ? "Device connected" : "Device disconnected");
  }

  // Update and send stats every 5 seconds 
  unsigned long currentMillis = millis();
  if (currentMillis - lastStatsUpdate >= 5000) {
    lastStatsUpdate = currentMillis;
    updateAndSendStats();
  }

  delay(100); // idle delay
}