#include "StatsCharacteristicHandler.h"
#include <Arduino.h>

extern bool statusRequested;

void StatsCharacteristicHandler::onWrite(BLECharacteristic* characteristic) {
  std::string value = characteristic->getValue();
  if (value == "STATUS") {
    statusRequested = true;
  }
}