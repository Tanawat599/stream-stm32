// Simple LoRa P2P wrapper using RadioLib
#ifndef LORAP2P_H
#define LORAP2P_H

#include <Arduino.h>
#include "config.h"
#include <RadioLib.h>

class LoRaP2P {
public:
  LoRaP2P();
  // initialize radio at given frequency (MHz)
  int16_t begin(float frequency);

  // send helpers
  int16_t send(const char* msg);
  int16_t sendBytes(const uint8_t* data, size_t len);

  // receive helpers
  String receive(uint32_t timeout = 1000);
  int16_t receiveBytes(uint8_t* buffer, size_t len, uint32_t timeout = 1000);

private:
  Module module;
  SX1278 radio;
};

#endif // LORAP2P_H
