#include "DeviceContext.h"

/**
 * Minimal main.cpp — all logic lives in DeviceContext and its
 * owned subsystems (WiFiManager, BLEManager).
 */

void setup() {
    DeviceContext::getInstance().setup();
}

void loop() {
    DeviceContext::getInstance().loop();
}