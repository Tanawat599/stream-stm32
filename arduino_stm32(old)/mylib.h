#ifndef MYLIB_H
#define MYLIB_H

#include <Arduino.h>
#include <Wire.h>
#include <LoRa.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// ===================== LoRa P2P =====================
// class LoRaP2P {
// public:
//   void begin(long frequency);

//   void send(const char* msg);
//   void sendBytes(uint8_t* data, size_t len);

//   bool available();
//   String receive();
//   int receiveBytes(uint8_t* buffer, size_t len);
// };

// ===================== LoRa WAN =====================
// class LoRaWan {
// public:
//   void begin();
//   void classC();
//   void classA();
// };

// ===================== Display =====================


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

class Display {
public:
    void begin(uint8_t address);
    void clear();
    void setCursor(uint8_t x, uint8_t y);
    void print(const char* msg);
    void printAt(uint8_t x, uint8_t y, const char* msg);
    void update();

private:
    Adafruit_SSD1306* _display;
};



// ===================== I2C =====================
class I2C {
public:
    static void slave_begin(uint8_t address);
    static void slave_loop();
    static void master_begin();
    static void master_send(uint8_t address, const char* msg);
    static void master_sendBytes(uint8_t address, uint8_t* data, size_t len);

private:
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