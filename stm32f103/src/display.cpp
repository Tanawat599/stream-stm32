#include <Wire.h>
#include <Arduino.h>
#include "mylib.h"
#include "config.h"
#define SLAVE_ADDR 0x08

const char msg[] = "HELLO from Master";


void Display::begin() {
  Wire.begin();   // Master
  Serial1.println("I2C Master Initialized");
}

void Display::show() {
  Wire.beginTransmission(SLAVE_ADDR);

  Wire.write((uint8_t*)msg, strlen(msg));

  Wire.endTransmission();
  
  Serial1.println("Message sent to Slave");
  delay(1000);
}
