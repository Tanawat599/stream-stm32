#ifndef MYLIB_H
#define MYLIB_H
#include <Arduino.h>
#include <Wire.h>

#ifndef LORA_P2P_H
#define LORA_P2P_H

#include <Arduino.h>
#include <LoRa.h>

class LoRaP2P {
public:
  void begin(long frequency);

  // Send
  void send(const char* msg);
  void sendBytes(uint8_t* data, size_t len);

  // Receive
  bool available();
  String receive();
  int receiveBytes(uint8_t* buffer, size_t len);

private:
};

#endif

class LoRaWan {
public:
    void begin();
    void classC();
    void classA();
};

void test();

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Wire.h>

class Display {
public:
  void begin(uint8_t address);

  // ===== Basic =====
  void clear();
  void setCursor(uint8_t col, uint8_t row);

  // ===== Print =====
  void print(const char* msg);
  void printAt(uint8_t col, uint8_t row, const char* msg);

private:
  uint8_t _addr;

  void sendCommand(uint8_t cmd);
  void sendData(const uint8_t* data, size_t len);
};

#endif

class I2C {
public:
  // ===== Slave =====
  void slave_begin(uint8_t address);
  void slave_loop();

  // ===== Master =====
  void master_begin();
  void master_send(uint8_t address, const char* msg);
  void master_sendBytes(uint8_t address, uint8_t* data, size_t len);

private:
  static void receiveEvent(int howMany);

  static volatile bool _newData;
  static char _buffer[20];
  static volatile int _idx;
};

// class RS485 {
// public:
//     void send_begin ();
//     void send_loop();
//     void receive_begin();
//     void receive_loop();
// };


class RS485 {
public:
  // Constructor 
  RS485(HardwareSerial& serial, int dePin, int rePin = -1);

  void begin(long baud);

  // Control mode
  void setTransmit();
  void setReceive();

  // Send
  void send(String data);
  void sendBytes(uint8_t* data, size_t len);

  // Receive
  String receive();
  int receiveBytes(uint8_t* buffer, size_t len);

  bool available();

private:
  HardwareSerial* _serial;
  int _dePin;
  int _rePin;
};

#endif