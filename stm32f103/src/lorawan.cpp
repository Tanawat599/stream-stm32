#include <RadioLib.h>
#include <Arduino.h>
#include "config.h"

SX1276 radio = new Module(PB11, PB0, PB12, PB1);
LoRaWANNode node(&radio, &AS923);

void LoRaWan::begin(){
    STM32_SERIAL.println(F("\nSetup LoRaWAN Class C..."));
    STM32_SERIAL.print(F("Initialise radio... "));

    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        STM32_SERIAL.println(F("failed!"));
        while (true);
    }
    STM32_SERIAL.println(F("success!"));

    STM32_SERIAL.print(F("Join Network... "));
    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);

    state = node.activateOTAA();
    if (state != RADIOLIB_LORAWAN_NEW_SESSION) {
        STM32_SERIAL.print(F("failed, code: ")); STM32_SERIAL.println(state);
        while (true);
    }
    STM32_SERIAL.println(F("Joined!"));

    node.setClass(RADIOLIB_LORAWAN_CLASS_C);

    const char* payload = "Hello";
    STM32_SERIAL.println(F("Sending first uplink..."));
    state = node.sendReceive((uint8_t*)payload, strlen(payload), 1, true);
}

void LoRaWan::classC(){
  uint8_t downlinkPayload[255];
  size_t downlinkLen = 0;
  LoRaWANEvent_t downlinkEvent;

  int16_t state = node.getDownlinkClassC(downlinkPayload, &downlinkLen, &downlinkEvent);
  
  if(state > 0 && downlinkLen > 0) {
    Serial1.print(F("Received (HEX): "));
    for (size_t i = 0; i < downlinkLen; i++) {
      if (downlinkPayload[i] < 0x10) Serial1.print('0');
      Serial1.print(downlinkPayload[i], HEX);
      Serial1.print(" ");
    }
    Serial1.println();

    Serial1.print(F("Received (Text): "));
    for (size_t i = 0; i < downlinkLen; i++) {
      if (isprint(downlinkPayload[i])) { 
        Serial1.print((char)downlinkPayload[i]);
      } else {
        Serial1.print('.');
      }
    }
    Serial1.println();
    Serial1.println(F("-------------------"));
  } 
  else {
    Serial1.print(F("Error receiving downlink, code: ")); Serial1.println(state);
    Serial1.println(F("-------------------"));
  }
}

