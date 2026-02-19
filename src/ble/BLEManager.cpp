#include "BLEManager.h"
#include "BLECallbacks.h"
#include "../DeviceContext.h"

BLEManager::BLEManager()
    : pServer(nullptr)
    , pService(nullptr)
    , pWiFiChar(nullptr)
    , pStatsChar(nullptr) {}

void BLEManager::begin(const String& deviceName) {
    BLEDevice::init(deviceName.c_str());

    // ── Server ───────────────────────────────────────────────────────
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new BLEServerHandler());

    // ── Service ──────────────────────────────────────────────────────
    pService = pServer->createService(SERVICE_UUID);

    // ── WiFi-config characteristic (write-only by client) ────────────
    pWiFiChar = pService->createCharacteristic(
        WIFI_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
    pWiFiChar->setCallbacks(new WiFiConfigCharacteristicHandler());

    // ── Stats characteristic (notify + read) ─────────────────────────
    pStatsChar = pService->createCharacteristic(
        STATS_CHAR_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);

    BLEDescriptor* cccd = new BLEDescriptor(BLEUUID((uint16_t)0x2902));
    pStatsChar->addDescriptor(cccd);

    // ── Start ────────────────────────────────────────────────────────
    pService->start();

    BLEAdvertising* adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(SERVICE_UUID);
    adv->setScanResponse(true);
    adv->setMinPreferred(0x06);
    BLEDevice::startAdvertising();

    Serial.println("[BLE] Advertising as \"" + deviceName + "\"");
}

void BLEManager::updateStats(const String& jsonStats) {
    if (!pStatsChar) return;
    pStatsChar->setValue(jsonStats.c_str());
    pStatsChar->notify();
}

bool BLEManager::isConnected() const {
    return DeviceContext::getInstance().getStats().isBluetoothConnected;
}
