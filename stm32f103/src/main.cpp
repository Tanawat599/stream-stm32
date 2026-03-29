#include <Arduino.h>
#include "mylib.h"
#include "config.h"

LoRaP2P lora;

void setup() {
  lora.begin(923E6);
}

void loop() {
  lora.send("Hello LoRa!");
  delay(2000);
}