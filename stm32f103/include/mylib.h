#ifndef MYLIB_H
#define MYLIB_H

#include <Arduino.h>
#include <Wire.h>

// #include <LoRa.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>
// ===================== LoRa P2P =====================
// class LoRaP2P {
// public:
//   void begin(float frequency);

//   void send(const char* msg);
//   void sendBytes(uint8_t* data, size_t len);

//   String receive();
//   int receiveBytes(uint8_t* buffer, size_t len);
// };

// ===================== LoRa WAN =====================
class LoRaWan {
public:
  void begin();
  void loop();
  void classC();
  // void classA();
};

// ===================== Display =====================
class Display {
public:
  void begin(uint8_t address);

  void clear();
  void setCursor(uint8_t x, uint8_t y);

  void print(const char* msg);
  void printAt(uint8_t x, uint8_t y, const char* msg);

  void update();

private:
  class Adafruit_SSD1306* _display; // forward declaration
};

// ===================== I2C =====================
class I2C {
public:
  void slave_begin(uint8_t address);
  void slave_loop();

  void master_begin();
  void master_send(uint8_t address, const char* msg);
  void master_sendBytes(uint8_t address, uint8_t* data, size_t len);

private:
  static void receiveEvent(int howMany);

  static volatile bool _newData;
  static char _buffer[20];
  static volatile int _idx;
};

// ===================== RS485 =====================
class RS485 {
public:
  RS485(HardwareSerial& serial, int dePin, int rePin = -1);

  void begin(long baud);

  void setTransmit();
  void setReceive();

  void send(String data);
  void sendBytes(uint8_t* data, size_t len);

  String receive();
  int receiveBytes(uint8_t* buffer, size_t len);

  bool available();

private:
  HardwareSerial* _serial;
  int _dePin;
  int _rePin;
};

#endif