#include <Wire.h>
#include <Arduino.h>
#include "mylib.h"
#include "config.h"

#define SLAVE_ADDR_1 0x08
#define SLAVE_ADDR_2 0x09

// ===== Load Config =====
void I2C::loadConfig(SDResourceManager& sd, const char* path) {
    String json = sd.readFile(path);
    if (json == "ERROR_OPEN") {
        Serial1.print(F("I2C: failed to open config " ));
        Serial1.println(path);
        return;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial1.println(F("I2C: JSON parse failed"));
        return;
    }

    JsonObject hardware = doc["hardware"].as<JsonObject>();
    if (!hardware.containsKey("i2c")) return;
    JsonObject i2c = hardware["i2c"].as<JsonObject>();
    if (!i2c.containsKey("frequency")) return;
    const int freq = i2c["frequency"].as<int>();
    Wire.setClock(freq);
    Serial1.print(F("I2C: Frequency set to ")); Serial1.print(freq); Serial1.println(F(" Hz"));
    const char* master_addr = i2c["master_address"].as<const char*>();
    Serial1.print(F("I2C: Master address set to ")); Serial1.println(master_addr);

}

// ===== Static variables =====
volatile bool I2C::_newData = false;
char I2C::_buffer[20];
volatile int I2C::_idx = 0;

// ===== Interrupt =====
void I2C::receiveEvent(int howMany) {
  _idx = 0;

  while (Wire.available() && _idx < sizeof(_buffer) - 1) {
    _buffer[_idx++] = Wire.read();
  }
  _buffer[_idx] = '\0';

  _newData = true;
}

// ===== Slave =====
void I2C::slave_begin(uint8_t address) {
  Wire.begin(address);
  Wire.onReceive(receiveEvent);
  Serial1.begin(115200);
  Serial1.println("I2C Slave Started");
}

void I2C::slave_loop() {
  if (_newData) {
    noInterrupts();

    char temp[20];
    strcpy(temp, _buffer);

    _newData = false;

    interrupts();

    Serial1.print("Received: ");
    Serial1.println(temp);
  }
}

// ===== Master =====
void I2C::master_begin() {
  Wire.begin();
  Serial1.begin(115200);
  Serial1.println("I2C Master Initialized");
}

void I2C::master_send(uint8_t address, const char* msg) {
  Wire.beginTransmission(address);
  Wire.write((uint8_t*)msg, strlen(msg));
  Wire.endTransmission();

  Serial1.print("Sent to 0x");
  Serial1.println(address, HEX);
}

void I2C::master_sendBytes(uint8_t address, uint8_t* data, size_t len) {
  Wire.beginTransmission(address);
  Wire.write(data, len);
  Wire.endTransmission();

  Serial1.print("Sent bytes to 0x");
  Serial1.println(address, HEX);
}

