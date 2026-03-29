#ifndef MYLIB_H
#define MYLIB_H
#include <Arduino.h>
#include <Wire.h>

class LoRaP2P {
public:
    void begin();
    void send();
    void receive();
};

class LoRaWan {
public:
    void begin();
    void classC();
    void classA();
};

void test();

class Display {
public:
    void begin();
    void show();
};

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