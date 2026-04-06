#ifndef MYLIB_H
#define MYLIB_H

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#pragma once

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


enum LoRaClassMode {
  CLASS_A,
  CLASS_C
};

class LoRaWan {
public:
  void begin();
  void loop();

  void setMode(LoRaClassMode mode);  
  void classC();
  void classA();                     

private:
  LoRaClassMode currentMode = CLASS_A;  
};

// ===================== Display =====================
// class Display {
// public:
//   void begin(uint8_t address);

//   void clear();
//   void setCursor(uint8_t x, uint8_t y);

//   void print(const char* msg);
//   void printAt(uint8_t x, uint8_t y, const char* msg);

//   void update();

// private:
//   class Adafruit_SSD1306* _display; // forward declaration
// };
class OLED {
public:
    // เริ่มต้นจอ
    void begin();

    // ล้างหน้าจอและอัปเดต
    void clear();
    void update();

    // ---------------- TEXT ----------------
    void setFont(const uint8_t* font);
    void print(const char* text, int x, int y);      // ระบุพิกัด
    void println(const char* text, int x, int y);    // ระบุพิกัด
    void drawStr(int x, int y, const char* text);

    // ---------------- DRAW ----------------
    void drawPixel(int x, int y);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawBox(int x, int y, int w, int h);
    void drawFrame(int x, int y, int w, int h);

    // ---------------- SETTINGS ----------------
    void setContrast(uint8_t value);
    void setFlip(bool flip);

    // ---------------- TEST ----------------
    void test();
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