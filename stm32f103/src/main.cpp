#include <Arduino.h>
#include "mylib.h"

LoRaWan lora;   // 👈 สร้าง object

void setup() {
  Serial1.begin(115200);

  lora.begin();   // 👈 เรียกใช้
}

void loop() {
  lora.classC();  // 👈 ฟัง downlink
}