#ifndef STATS_CHARACTERISTIC_HANDLER_H
#define STATS_CHARACTERISTIC_HANDLER_H

#include <BLECharacteristic.h>

class StatsCharacteristicHandler : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override;
};

#endif // STATS_CHARACTERISTIC_HANDLER_H