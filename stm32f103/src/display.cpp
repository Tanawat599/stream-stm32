#include <Wire.h>
#include <Arduino.h>
#include "mylib.h"
#include "config.h"
// ===== Init =====
void Display::begin(uint8_t address) {
  _addr = address;
  Wire.begin();
  Serial1.println("Display I2C Initialized");
}

// ===== Low level =====
void Display::sendCommand(uint8_t cmd) {
  Wire.beginTransmission(_addr);
  Wire.write(0x00);    
  Wire.write(cmd);
  Wire.endTransmission();
}

void Display::sendData(const uint8_t* data, size_t len) {
  Wire.beginTransmission(_addr);
  Wire.write(0x40);     

  for (size_t i = 0; i < len; i++) {
    Wire.write(data[i]);
  }

  Wire.endTransmission();
}

// ===== Basic Functions =====
void Display::clear() {
  sendCommand(0x01);  
  delay(2);
}

void Display::setCursor(uint8_t col, uint8_t row) {
  uint8_t addr = col + (row == 0 ? 0x00 : 0x40);
  sendCommand(0x80 | addr);
}

// ===== Print =====
void Display::print(const char* msg) {
  sendData((uint8_t*)msg, strlen(msg));
}

void Display::printAt(uint8_t col, uint8_t row, const char* msg) {
  setCursor(col, row);
  print(msg);
}